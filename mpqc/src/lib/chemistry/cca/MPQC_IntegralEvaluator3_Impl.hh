// 
// File:          MPQC_IntegralEvaluator3_Impl.hh
// Symbol:        MPQC.IntegralEvaluator3-v0.2
// Symbol Type:   class
// Babel Version: 0.9.8
// Description:   Server-side implementation for MPQC.IntegralEvaluator3
// 
// WARNING: Automatically generated; only changes within splicers preserved
// 
// babel-version = 0.9.8
// 

#ifndef included_MPQC_IntegralEvaluator3_Impl_hh
#define included_MPQC_IntegralEvaluator3_Impl_hh

#ifndef included_sidl_cxx_hh
#include "sidl_cxx.hh"
#endif
#ifndef included_MPQC_IntegralEvaluator3_IOR_h
#include "MPQC_IntegralEvaluator3_IOR.h"
#endif
// 
// Includes for all method dependencies.
// 
#ifndef included_Chemistry_QC_GaussianBasis_Molecular_hh
#include "Chemistry_QC_GaussianBasis_Molecular.hh"
#endif
#ifndef included_MPQC_IntegralEvaluator3_hh
#include "MPQC_IntegralEvaluator3.hh"
#endif
#ifndef included_sidl_BaseInterface_hh
#include "sidl_BaseInterface.hh"
#endif
#ifndef included_sidl_ClassInfo_hh
#include "sidl_ClassInfo.hh"
#endif


// DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator3._includes)
// Put additional includes or other arbitrary code here...
// DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator3._includes)

namespace MPQC { 

  /**
   * Symbol "MPQC.IntegralEvaluator3" (version 0.2)
   */
  class IntegralEvaluator3_impl
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator3._inherits)
  // Put additional inheritance here...
  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator3._inherits)
  {

  private:
    // Pointer back to IOR.
    // Use this to dispatch back through IOR vtable.
    IntegralEvaluator3 self;

    // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator3._implementation)
    // Put additional implementation details here...
    // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator3._implementation)

  private:
    // private default constructor (required)
    IntegralEvaluator3_impl() {} 

  public:
    // sidl constructor (required)
    // Note: alternate Skel constructor doesn't call addref()
    // (fixes bug #275)
    IntegralEvaluator3_impl( struct MPQC_IntegralEvaluator3__object * s ) : 
      self(s,true) { _ctor(); }

    // user defined construction
    void _ctor();

    // virtual destructor (required)
    virtual ~IntegralEvaluator3_impl() { _dtor(); }

    // user defined destruction
    void _dtor();

  public:

    /**
     * user defined non-static method.
     */
    void
    set_integral_package (
      /*in*/ const ::std::string& label
    )
    throw () 
    ;


    /**
     * Initialize the evaluator.
     * @param bs1 Molecular basis on center 1.
     * @param bs2 Molecular basis on center 2.
     * @param bs3 Molecular basis on center 3.
     * @param label String specifying integral type.
     * @param max_deriv Max derivative to compute. 
     */
    void
    initialize (
      /*in*/ ::Chemistry::QC::GaussianBasis::Molecular bs1,
      /*in*/ ::Chemistry::QC::GaussianBasis::Molecular bs2,
      /*in*/ ::Chemistry::QC::GaussianBasis::Molecular bs3,
      /*in*/ const ::std::string& label,
      /*in*/ int64_t max_deriv
    )
    throw () 
    ;


    /**
     * Get the buffer pointer
     * @return Buffer pointer 
     */
    void*
    get_buffer() throw () 
    ;

    /**
     * Compute a shell triplet of integrals.
     * @param shellnum1 Gaussian shell number 1.
     * @param shellnum2 Gaussian shell number 2.
     * @param shellnum3 Gaussian shell number 3.
     * @param deriv_level Derivative level. 
     */
    void
    compute (
      /*in*/ int64_t shellnum1,
      /*in*/ int64_t shellnum2,
      /*in*/ int64_t shellnum3,
      /*in*/ int64_t deriv_level
    )
    throw () 
    ;


    /**
     * Compute a shell triplet of integrals and return as a borrowed
     * sidl array.
     * @param shellnum1 Gaussian shell number 1.
     * @param shellnum2 Gaussian shell number 2.
     * @param shellnum3 Gaussian shell number 3.
     * @param deriv_level Derivative level.
     * @return Borrowed sidl array buffer. 
     */
    ::sidl::array<double>
    compute_array (
      /*in*/ int64_t shellnum1,
      /*in*/ int64_t shellnum2,
      /*in*/ int64_t shellnum3,
      /*in*/ int64_t deriv_level
    )
    throw () 
    ;

  };  // end class IntegralEvaluator3_impl

} // end namespace MPQC

// DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator3._misc)
// Put miscellaneous things here...
// DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator3._misc)

#endif
