#ifndef MPQC_FILE_HPP
#define MPQC_FILE_HPP

#include <string>
#include <cassert>
#include <fstream>

#include <stdlib.h>
#include <hdf5.h>

#include <memory>

#include <boost/array.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/noncopyable.hpp>
#include <boost/type_traits.hpp>
#include <boost/assert.hpp>

#include <boost/typeof/typeof.hpp>

#include "mpqc/range.hpp"
#include "mpqc/range/operator.hpp"

#include "mpqc/assert.hpp"
#include "mpqc/utility/foreach.hpp"
#include "mpqc/utility/timer.hpp"
#include "mpqc/utility/mutex.hpp"

#include <util/misc/exenv.h>

/**
 * @defgroup File mpqc.Core.File
 * Implementation of hierarchical file objects based on
 * <a href="http://www.hdfgroup.org/HDF5/">HDF5</a>.
*/

namespace mpqc {
namespace detail {

/** @brief implementation details
    @ingroup File
*/
namespace File {

/// Verify HDF5 operation was success
#define MPQC_FILE_VERIFY(expr) MPQC_CHECK((expr) >= 0)

#ifndef H5_HAVE_THREADSAFE
#warning "HDF5 NOT THREADSAFE, HDF5 will use global mutex"
#endif

    /// HDF5 may not be threadsafe, in that case mpqc::mutex::global is used
    struct threadsafe : boost::noncopyable {
        //#ifndef H5_HAVE_THREADSAFE
        threadsafe() {
            // use global lock since HDF5 may call MPI routines
            lock();
        }
        ~threadsafe() {
            unlock();
        }
        static void lock() { mutex::global::lock(); }
        static void unlock() { mutex::global::unlock(); }
        //#endif
    };

#define MPQC_FILE_THREADSAFE mpqc::detail::File::threadsafe _threadsafe

    /**
       Translate C type into corresponding HDF5 type
    */
    template<typename T>
    inline hid_t h5t() {
        typedef typename boost::remove_const<T>::type U;
        if (boost::is_same<U,int>::value) return H5T_NATIVE_INT;
        if (boost::is_same<U,double>::value) return H5T_NATIVE_DOUBLE;
        throw std::runtime_error("no mapping to HDF5 type");
        return hid_t();
    }


    /**
       A reference-counted HDF5 handle object,
       superclass for eg File, Dataset, Attribute, etc
     */
    struct Object {

        /** Default constructor with an invalid handle */
        Object() : id_(0) {}

        /** Copy constructor */
        Object(const Object &o) {
            *this = o;
        }

        /**
           @param parent Parent object
           @param id HDF5 id
           @param close Function pointer called when reference count reaches 0
           @param increment Flag to increment (or not) the reference count
         */
        Object(const Object &parent, hid_t id, void (*close)(hid_t), bool increment) {
            assert(id);
            parent_.reset(new Object(parent));
            update(id, close, increment);
        }

        ~Object() {
            if (!id_) return;
            assert(close_);
            MPQC_FILE_THREADSAFE;
            close_(id_);
        }

        void operator=(const Object &o) {
            Object *parent = o.parent_.get();
            parent_.reset(parent ? new Object(*parent) : NULL);
            update(o.id_, o.close_, true);
        }

        hid_t id() const {
            return id_;
        }

        const Object& parent() const {
            return *parent_;
        }

        hid_t file() const {
            MPQC_FILE_THREADSAFE;
            return H5Iget_file_id(id_);
        }

        static std::string filename(hid_t id) {
            MPQC_FILE_THREADSAFE;
            std::vector<char> str(H5Fget_name(id, NULL, 0) + 1);
            MPQC_FILE_VERIFY(H5Fget_name(id, &str[0], str.size()));
            return std::string(&str[0]);
        }

        operator bool() const {
            return (this->id_);
        }

    protected:

        std::auto_ptr<Object> parent_;
        hid_t id_;
        void (*close_)(hid_t);

        template<class F>
        void update(hid_t id, F close, bool increment) {
            if (id && increment) {
                MPQC_FILE_THREADSAFE;
                MPQC_FILE_VERIFY(H5Iinc_ref(id));
            }
            id_ = id;
            close_ = close;
        }
    };

    struct Attribute: Object {
    };

    template<class Container>
    struct static_container {
        static Container data;
    };
    
    template<class Container>
    Container static_container<Container>::data;

} // namespace File
} // namespace detail
} // namespace mpqc

namespace mpqc {

    /** @addtogroup File
        @{ */

    /**
       Top-level file object that holds groups.
       All I/O is done through Dataspace objects which belong to group.
       The file storage and access properties are determined by the Driver.
       File objects are copyable, referenced-counted using HDF5 Identifier API
     */
    struct File: detail::File::Object {

        typedef detail::File::Object Object;
        typedef detail::File::Attribute Attribute;
        
        struct Group;

        template<typename T>
        struct Dataset;

        template<typename T>
        struct Dataspace;

        /**
           Default file driver
           @warning This class needs work to accomodate different HDF5 drivers better
         */
        struct Driver {
            struct Core;
            Driver() : fapl_(H5P_DEFAULT) {}
            hid_t fapl() const {
                return fapl_;
            }
        private:
            hid_t fapl_;
        };

        /**
           Constructs a null file object.
           Creating objects with this file as parent will fail
         */
        File() {}

        /**
           Create or open File.
           File may be opened multiple times: if an open file by that name
           already exists, that existing instance will be used.
           @param[in] name file name
           @param[in] driver File driver, determining how file is created.
           @warning NOT threadsafe
           @details The list of opened files is stored internally in a static set
        */
        explicit File(const std::string &name, const Driver &driver = Driver()) {
            initialize(name, driver);
        }

        /**
           Creates or opens a file group.
           @param[in] name group name. Default argument implies default group.
           @warning NOT threadsafe
        */
        Group group(const std::string &name = "/");

    private:

        // opened files
        typedef detail::File::static_container< std::set<hid_t> > files_;

        /// Initialize (open or create) file
        void initialize(const std::string &name, const Driver &driver) {
            Object o = File::open(name, driver);
            if (!o) {
                o = File::create(name, driver);
            }
            Object::operator=(o);
        }

        /**
           @warning NOT threadsafe
         */
        static void close(hid_t id) {
            hid_t count, result;
            MPQC_FILE_VERIFY((count = H5Iget_ref(id)));
            MPQC_FILE_VERIFY(result = H5Fclose(id));
            if (!(count-1)) {
                files_::data.erase(id);
            }
        }

        explicit File(hid_t id, bool increment) :
            Object(Object(), id, &File::close, increment)
        {
            MPQC_FILE_VERIFY(id);
        }

        /**
           Translate name into *filesystem* realpath
           If file doesn't exists, it returns name argument unchanged
        */
        static std::string realpath(const std::string &name) {
            char *str = ::realpath(name.c_str(), NULL);
            //std::cout << "realpath: " << str << std::endl;
            std::string path(str ? str : name);
            free(str);
            return path;
        }
        
        /**
           Open file using driver FAPL.  If file doesn't exists, it will be created.
           If name is empty, an null File will be returned.
           @param name File name
           @param driver Driver
           @warning NOT threadsafe
        */
        static File open(const std::string &name, const Driver &driver) {
            if (name.empty()) return File();
            std::string path = realpath(name);
            // find previously opened file of same realpath
            foreach (auto id, files_::data) {
                if (path == realpath(Object::filename(id)))
                    return File(id, true);
            }
            hid_t fapl = driver.fapl();
            hid_t id = H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
            MPQC_FILE_VERIFY(id);
            files_::data.insert(id);
            return File(id, false);
            //return File(H5Fopen(path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT), false);
        }

        /**
           Create file using driver FAPL.  If file exists, the call will fail.
           @param name File name
           @param driver Driver
           @warning NOT threadsafe
         */
        static File create(const std::string &name, const Driver &driver) {
            hid_t fapl = driver.fapl();
            hid_t id = H5Fcreate(name.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, fapl);
            MPQC_FILE_VERIFY(id);
            files_::data.insert(id);
            return File(id, false);
        }

    };

    /**
       A subset of File::Dataset
       @todo Fix const version
    */
    template<typename T_>
    struct File::Dataspace {

        typedef typename boost::remove_const<T_>::type T;

        /** Number of elements in the set */
        size_t size() const {
            return size_;
        }

        /** The extents of the set in terms of ranges */
        const std::vector<range>& extents() const {
            return range_;
        }

        /** Access sub-dataspace of rank-1 */
        Dataspace<T> operator[](size_t i) {
            std::vector<range> r = range_;
            r[ndims_ - 1] = range(i, i + 1);
            return Dataspace(parent_, base_, r, ndims_ - 1);
        }

        /** Access sub-dataspace of the same rank */
        Dataspace<T> operator()(const std::vector<range> &r) {
            return Dataspace<T>(parent_, base_, extend(r), ndims_);
        }

#ifdef DOXYGEN
        /**
           N-ary sub-dataspace access operators.
           The parameters R should be either integral types (a single element)
           or of type mpqc::range (a range of elements)
           The method packs arguments into <c>std::vector<range></c>
           and calls the equivalent operator.
        */
        template<class R, ...>
        Dataspace<T> operator()(const R &r, ...);
#else
        template<class S>
        Dataspace<T> operator()(const range::tie<S> &t) {
            return this->operator()(std::vector<range>(t));
        }
        MPQC_RANGE_OPERATORS(4, Dataspace<T>, this->operator())
#endif

        /**
           Writes contiguous buffer into dataset.
           The size of buffer must be the same as size of dataspace
           This function is threadsafe
         */
        void write(const T *buffer) {
            MPQC_PROFILE_LINE;
            timer t;
            apply(&H5Dwrite, this->parent_.id(), rebase(range_), (T*) buffer);
            // printf("File::write size=%lu bytes, %f mb/s\n",
            //        (sizeof(T)*size_), (sizeof(T)*size_)/(t*1e6));
        }

        /**
           Reads contiguous buffer from dataset.
           The size of buffer must be the same as size of dataspace
           This function is threadsafe
         */
        void read(T *buffer) const {
            MPQC_PROFILE_LINE;
            timer t;
            apply(&H5Dread, this->parent_.id(), rebase(range_), buffer);
            // printf("File::read size=%lu bytes, rate=%f mb/s\n",
            //        (sizeof(T)*size_), (sizeof(T)*size_)/(t*1e6));
        }

    private:

        Object parent_;
        size_t ndims_, size_;
        std::vector<size_t> base_;
        std::vector<range> range_;
        friend class Dataset<T> ;

        Dataspace(const Object &parent, const std::vector<size_t> &base,
                  const std::vector<range> &r, size_t ndims) :
            parent_(parent),
            base_(base),
            range_(r),
            ndims_(ndims)
        {
            assert(ndims <= r.size());
            size_ = (r.size() ? 1 : 0);
            for (int i = 0; i < r.size(); ++i) {
                //std::cout << "r = " << r[i] << std::endl;
                size_ *= r[i].size();
            }
        }

        /// extend ranges to the full rank of the parent <i>dataset</i> 
        std::vector<range> extend(const std::vector<range> &r) const {
            //std::cout << r.size() << " " << ndims_ << std::endl;
            assert(r.size() == ndims_);
            std::vector<range> x = r;
            for (size_t i = ndims_; i < range_.size(); ++i) {
                x.push_back(range_[i]);
            }
            return x;
        }

        /// shift ranges to match layout of the parent <i>dataset</i>
        std::vector<range> rebase(const std::vector<range> &r) const {
            assert(r.size() == base_.size());
            std::vector<range> v;
            for (int i = 0; i < base_.size(); ++i) {
                int begin = *r[i].begin() - base_[i];
                v.push_back(range(begin, begin + r[i].size()));
            }
            return v;
        }

        /// Apply read/write function to subset r of HDF5 dset
        template<class F>
        static void apply(F f, hid_t dset, const std::vector<range> &r, T *buffer) {
            MPQC_FILE_THREADSAFE;
            hid_t fspace = H5Dget_space(dset);
            size_t size = select(fspace, r);
            hsize_t mdims[] = { size };
            hid_t mspace = H5Screate_simple(1, mdims, NULL);
            MPQC_FILE_VERIFY(mspace);
            MPQC_FILE_VERIFY(H5Sselect_all(mspace));
            hid_t type = detail::File::h5t<T>();
            timer t;
            MPQC_FILE_VERIFY(f(dset, type, mspace, fspace, H5P_DEFAULT, buffer));
            MPQC_FILE_VERIFY(H5Sclose(mspace));
            MPQC_FILE_VERIFY(H5Sclose(fspace));
        }

        /// @warning NOT threadsafe.  Should only be called by apply
        static size_t select(hid_t space, const std::vector<range> &r) {
            size_t N = H5Sget_simple_extent_ndims(space);
            MPQC_FILE_VERIFY(N);
            //printf("id = %i, N = %lu\n", space, N);
            hsize_t fstart[N];
            hsize_t fstride[N]; // Stride of hyperslab
            hsize_t fcount[N]; // Block count
            hsize_t fblock[N]; // Block sizes
            size_t size = 1;
            //printf("select [ ");
            for (size_t i = 0, j = N - 1; i < N; ++i, --j) {
                fstart[i] = *r[j].begin();
                fcount[i] = r[j].size();
                //printf("%i:%i,", fstart[i], fstart[i]+fcount[i]);
                fstride[i] = 1;
                fblock[i] = 1;
                size *= fcount[i];
            }
            //printf(" ], size = %i\n", size);
            MPQC_FILE_VERIFY
                (H5Sselect_hyperslab
                 (space, H5S_SELECT_SET, fstart, fstride, fcount, fblock));
            return size;
        }

    };

    /**
       Array-like collection of data
    */
    template<typename T>
    struct File::Dataset : File::Object {

        Dataset() {}

        /** Create dataset belonging to some file object
            @param parent Dataset parent
            @param name Dataset name
            @param extents Dataset extents.
            Extents must be a collection of dataset dimensions or ranges.
            The size of collection determines dataset rank.
        */
        template<typename Extents>
        Dataset(const Object &parent, const std::string &name,
                const Extents &extents, const std::vector<size_t> &chunk =
                std::vector<size_t>())
            : Object(Dataset::create(parent, name, extents, chunk))
        {
            assert(id() > 0);
            foreach (auto e, extents) {
                range r = extent(e);
                std::cout << "Dataset " << r << std::endl;
                base_.push_back(*r.begin());
                dims_.push_back(r.size());
            }
        }

        /** Dataspace rank (number of dimensions) */
        size_t rank() const {
            return dims_.size();
        }

        /** Access dataspace of rank-1 */
        Dataspace<T> operator[](size_t index) {
            std::vector<range> r;
            for (int i = 0; i < this->rank(); ++i) {
                r.push_back(range(base_[i], base_[i] + dims_[i]));
            }
            return Dataspace<T>(*this, base_, r, r.size())[index];
        }

        /** Access dataspace of same rank */
        Dataspace<T> operator()(const std::vector<range> &r) {
            //std::cout << this->extents_.size() << " " << r.size() << std::endl;
            assert(this->rank() == r.size());
            return Dataspace<T>(*this, base_, r, r.size());
        }

        /** Access dataspace of same rank */
        Dataspace<const T> operator()(const std::vector<range> &r) const {
            assert(this->rank() == r.size());
            return Dataspace<const T>(*this, base_, r, r.size());
        }

#ifdef DOXYGEN
        /**
           N-ary sub-dataspace access operators.
           The parameters R should be either integral types (a single element)
           or of type mpqc::range (a range of elements)
           The method packs arguments into <c>std::vector<range></c>
           and calls the equivalent operator.
        */
        template<class R, ...>
        Dataspace<T> operator()(const R &r, ...);
#else
        template<class S>
        Dataspace<T> operator()(const range::tie<S> &t) {
            return this->operator()(std::vector<range>(t));
        }
        template<class S>
        Dataspace<T> operator()(const range::tie<S> &t) const {
            return this->operator()(std::vector<range>(t));
        }
        MPQC_RANGE_OPERATORS(4, Dataspace<T>, this->operator())
        MPQC_RANGE_CONST_OPERATORS(4, Dataspace<const T>, this->operator())
#endif

        template<typename Extents>
        static Object create(const Object &parent,
                             const std::string &name,
                             const Extents &extents,
                             const std::vector<size_t> &chunk) {

            hid_t id;
            // Object constructor may also obtain mutex
            // make sure lock is released before constructing Object
            {
                MPQC_FILE_THREADSAFE;
                std::vector<hsize_t> dims;
                foreach (auto e, extents) {
                    dims.push_back(extent(e).size());
                    std::cout << dims.back() << std::endl;
                }
                std::reverse(dims.begin(), dims.end());

                hid_t fspace = H5Screate_simple(dims.size(), &dims[0], NULL);
                hid_t type = detail::File::h5t<T>();
                hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
                if (chunk.size()) {
                    assert(chunk.size() == dims.size());
                    std::vector<hsize_t> block(chunk.rbegin(), chunk.rend());
                    H5Pset_chunk(dcpl, block.size(), &block[0]);
                }
                //printf("parent.id() = %i, name=%s\n", parent.id(), name.c_str());
#if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
                id = H5Dcreate(parent.id(), name.c_str(), type, fspace, dcpl);
#else
                id = H5Dcreate1(parent.id(), name.c_str(), type, fspace, dcpl);
#endif
                MPQC_FILE_VERIFY(id);
                MPQC_FILE_VERIFY(H5Pclose(dcpl));
            }
            return Object(parent, id, &Dataset::close, false);
        }

        // static Dataset open(Object parent, const std::string &name) {
        //     return Dataset(H5Dopen1(parent.id(), name.c_str()));
        // }

    protected:

        /** Return range as is */
        static range extent(const range &r) {
            return r;
        }

        /** Return integral argument e as range(0,e) */
        template <typename E>
        static range extent(const E &e) {
            return range(0,e);
        }

    private:

        std::vector<size_t> base_; // base index
        std::vector<size_t> dims_; // dimensions

        /**
           @warning NOT threadsafe
        */
        static void close(hid_t id) {
            // printf("H5Dclose(%i), ref=%i\n", id, H5Iget_ref(id));
            MPQC_FILE_VERIFY(H5Dclose(id));
        }

    };

    /**
       Directory-like container that holds datasets and other groups
    */
    struct File::Group: File::Object {

        /**
           Create a group
           @param parent group parent
           @param name group name
           @warning NOT threadsafe
         */
        static Group create(Object parent, const std::string &name) {
#if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
            hid_t id = H5Gopen(parent.id(), name.c_str());
#else
            hid_t id = H5Gopen1(parent.id(), name.c_str());
#endif
            return Group(Object(parent, id, &Group::close, false));
        }

        /**
           Create or open Dataset
           @tparam T Dataset type
           @param name Dataset name
           @param dims Dataset dimensions
        */
        template<typename T, typename Dims>
        Dataset<T> dataset(const std::string &name, const Dims &dims) {
            return Dataset<T>(*this, name, dims);
        }

    private:

        /**
           @warning NOT threadsafe
         */
        static void close(hid_t id) {
            //printf("H5Gclose(%i), ref=%i\n", id, H5Iget_ref(id));
            MPQC_FILE_VERIFY(H5Gclose(id));
        }

        Group(Object o) : Object(o) {}
    };

    inline File::Group File::group(const std::string &name) {
        return Group::create(*this, name);
    }

    /**
       Read from dataspace into a generic container A.
       @tparam A Container with member <c>T* A::data()</c>
       @param ds Dataspace to read from
       @param a Container to read to.
       @warning The pointer returned by A::data() must be contigous
    */
    template<typename T, class A>
    void operator>>(File::Dataspace<T> ds, A &a) {
        ds.read(a.data());
    }

    /**
       Read to dataspace from a generic container A.
       @tparam A Container with member <c>const T* A::data()</c>
       @param ds Dataspace to read from
       @param a Container to read to.
       @warning The pointer returned by A::data() must be contigous
    */
    template<typename T, class A>
    void operator<<(File::Dataspace<T> ds, const A &a) {
        ds.write(a.data());
    }

    /** @} */ // group File

} // namespace mpqc

#endif // MPQC_FILE_HPP
