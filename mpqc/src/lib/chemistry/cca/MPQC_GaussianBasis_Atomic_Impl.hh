// 
// File:          MPQC_GaussianBasis_Atomic_Impl.hh
// Symbol:        MPQC.GaussianBasis_Atomic-v0.2
// Symbol Type:   class
// Babel Version: 0.9.8
// Description:   Server-side implementation for MPQC.GaussianBasis_Atomic
// 
// WARNING: Automatically generated; only changes within splicers preserved
// 
// babel-version = 0.9.8
// 

#ifndef included_MPQC_GaussianBasis_Atomic_Impl_hh
#define included_MPQC_GaussianBasis_Atomic_Impl_hh

#ifndef included_sidl_cxx_hh
#include "sidl_cxx.hh"
#endif
#ifndef included_MPQC_GaussianBasis_Atomic_IOR_h
#include "MPQC_GaussianBasis_Atomic_IOR.h"
#endif
// 
// Includes for all method dependencies.
// 
#ifndef included_Chemistry_QC_GaussianBasis_AngularType_hh
#include "Chemistry_QC_GaussianBasis_AngularType.hh"
#endif
#ifndef included_Chemistry_QC_GaussianBasis_Shell_hh
#include "Chemistry_QC_GaussianBasis_Shell.hh"
#endif
#ifndef included_MPQC_GaussianBasis_Atomic_hh
#include "MPQC_GaussianBasis_Atomic.hh"
#endif
#ifndef included_sidl_BaseInterface_hh
#include "sidl_BaseInterface.hh"
#endif
#ifndef included_sidl_ClassInfo_hh
#include "sidl_ClassInfo.hh"
#endif


// DO-NOT-DELETE splicer.begin(MPQC.GaussianBasis_Atomic._includes)
#include <chemistry/qc/basis/basis.h>
#include <MPQC_GaussianBasis_Shell.hh>
using namespace std;
using namespace Chemistry::QC::GaussianBasis;
using namespace sc;
using namespace MPQC;
// Put additional includes or other arbitrary code here...
// DO-NOT-DELETE splicer.end(MPQC.GaussianBasis_Atomic._includes)

namespace MPQC { 

  /**
   * Symbol "MPQC.GaussianBasis_Atomic" (version 0.2)
   */
  class GaussianBasis_Atomic_impl
  // DO-NOT-DELETE splicer.begin(MPQC.GaussianBasis_Atomic._inherits)
  // Put additional inheritance here...
  // DO-NOT-DELETE splicer.end(MPQC.GaussianBasis_Atomic._inherits)
  {

  private:
    // Pointer back to IOR.
    // Use this to dispatch back through IOR vtable.
    GaussianBasis_Atomic self;

    // DO-NOT-DELETE splicer.begin(MPQC.GaussianBasis_Atomic._implementation)
    
    GaussianBasisSet *gbs_ptr_;
    Ref<GaussianBasisSet> sc_gbs_;
    int atomnum_;
    int nshell_;
    int max_am_;
    GaussianBasis_Shell *shell_array_;
    AngularType angular_type_;

    // DO-NOT-DELETE splicer.end(MPQC.GaussianBasis_Atomic._implementation)

  private:
    // private default constructor (required)
    GaussianBasis_Atomic_impl() {} 

  public:
    // sidl constructor (required)
    // Note: alternate Skel constructor doesn't call addref()
    // (fixes bug #275)
    GaussianBasis_Atomic_impl( struct MPQC_GaussianBasis_Atomic__object * s ) : 
      self(s,true) { _ctor(); }

    // user defined construction
    void _ctor();

    // virtual destructor (required)
    virtual ~GaussianBasis_Atomic_impl() { _dtor(); }

    // user defined destruction
    void _dtor();

  public:

    /**
     * user defined non-static method.
     */
    void
    initialize (
      /*in*/ void* scbasis,
      /*in*/ int32_t atomnum
    )
    throw () 
    ;


    /**
     * Get the canonical basis set name. 
     * @return Canonical basis set name. 
     */
    ::std::string
    get_name() throw () 
    ;

    /**
     * Get the number of basis functions.
     * @return Number of basis functions. 
     */
    int64_t
    get_n_basis() throw () 
    ;

    /**
     * Get the number of shells.
     * @return Number of shells. 
     */
    int64_t
    get_n_shell() throw () 
    ;

    /**
     * Get the max angular momentum for any shell on the atom.
     * @return Max angular momentum value. 
     */
    int64_t
    get_max_angular_momentum() throw () 
    ;

    /**
     * Get the angular type for the atom.
     * @return enum AngularType {CARTESIAN,SPHERICAL,MIXED} 
     */
    ::Chemistry::QC::GaussianBasis::AngularType
    get_angular_type() throw () 
    ;

    /**
     * Get a gaussian shell. 
     * @param shellnum Shell number to return.
     * @return Shell. 
     */
    ::Chemistry::QC::GaussianBasis::Shell
    get_shell (
      /*in*/ int64_t shellnum
    )
    throw () 
    ;


    /**
     * Print the atomic basis data. 
     */
    void
    print_atomic() throw () 
    ;
  };  // end class GaussianBasis_Atomic_impl

} // end namespace MPQC

// DO-NOT-DELETE splicer.begin(MPQC.GaussianBasis_Atomic._misc)
// Put miscellaneous things here...
// DO-NOT-DELETE splicer.end(MPQC.GaussianBasis_Atomic._misc)

#endif
