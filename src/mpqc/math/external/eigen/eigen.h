#ifndef MPQC4_SRC_MPQC_MATH_EXTERNAL_EIGEN_EIGEN_H_
#define MPQC4_SRC_MPQC_MATH_EXTERNAL_EIGEN_EIGEN_H_

#pragma GCC diagnostic push
#pragma GCC system_header
#include <Eigen/Dense>
#pragma GCC diagnostic pop

namespace mpqc {
template <typename T>
using RowMatrix =
    ::Eigen::Matrix<T, ::Eigen::Dynamic, ::Eigen::Dynamic, ::Eigen::RowMajor>;

using RowMatrixXd = RowMatrix<double>;
using Vector3d = Eigen::Vector3d;
using Vector3i = Eigen::Vector3i;
}

namespace madness {
namespace archive {

template <class>
class archive_array;
template <class T>
inline archive_array<T> wrap(const T*, unsigned int);
template <class Archive, typename Data>
struct ArchiveStoreImpl;
template <class Archive, typename Data>
struct ArchiveLoadImpl;

template <class Archive, typename _T>
struct ArchiveStoreImpl<Archive, mpqc::RowMatrix<_T>> {
  static inline void store(const Archive& ar, const mpqc::RowMatrix<_T>& t) {
    ar& t.rows() & t.cols();
    if (t.size()) ar& madness::archive::wrap(t.data(), t.size());
  }
};

template <class Archive, typename _T>
struct ArchiveLoadImpl<Archive, mpqc::RowMatrix<_T>> {
  static inline void load(const Archive& ar, mpqc::RowMatrix<_T>& t) {
    typename mpqc::RowMatrix<_T>::Index nrows(0), ncols(0);
    ar& nrows& ncols;
    t.resize(nrows, ncols);
    if (t.size()) ar& madness::archive::wrap(t.data(), t.size());
  }
};

}  // namespace archive
}  // namespace madness

#endif  // MPQC4_SRC_MPQC_MATH_EXTERNAL_EIGEN_EIGEN_H_
