// 
// File:          MPQC_IntegralEvaluator4_Impl.hh
// Symbol:        MPQC.IntegralEvaluator4-v0.2
// Symbol Type:   class
// Babel Version: 0.9.8
// Description:   Server-side implementation for MPQC.IntegralEvaluator4
// 
// WARNING: Automatically generated; only changes within splicers preserved
// 
// babel-version = 0.9.8
// 

#ifndef included_MPQC_IntegralEvaluator4_Impl_hh
#define included_MPQC_IntegralEvaluator4_Impl_hh

#ifndef included_sidl_cxx_hh
#include "sidl_cxx.hh"
#endif
#ifndef included_MPQC_IntegralEvaluator4_IOR_h
#include "MPQC_IntegralEvaluator4_IOR.h"
#endif
// 
// Includes for all method dependencies.
// 
#ifndef included_Chemistry_QC_GaussianBasis_Molecular_hh
#include "Chemistry_QC_GaussianBasis_Molecular.hh"
#endif
#ifndef included_MPQC_IntegralEvaluator4_hh
#include "MPQC_IntegralEvaluator4.hh"
#endif
#ifndef included_sidl_BaseInterface_hh
#include "sidl_BaseInterface.hh"
#endif
#ifndef included_sidl_ClassInfo_hh
#include "sidl_ClassInfo.hh"
#endif


// DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator4._includes)
#include <chemistry/qc/basis/gaussbas.h>
#include <chemistry/qc/intv3/intv3.h>
#ifdef HAVE_CINTS
  #include <chemistry/qc/cints/cints.h>
#endif
using namespace sc;
// DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator4._includes)

namespace MPQC { 

  /**
   * Symbol "MPQC.IntegralEvaluator4" (version 0.2)
   */
  class IntegralEvaluator4_impl
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator4._inherits)
  // Put additional inheritance here...
  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator4._inherits)
  {

  private:
    // Pointer back to IOR.
    // Use this to dispatch back through IOR vtable.
    IntegralEvaluator4 self;

    // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator4._implementation)
    Chemistry::Molecule molecule_;
    std::string evaluator_label_;
    Ref<sc::GaussianBasisSet> bs1_;
    Ref<sc::GaussianBasisSet> bs2_;
    Ref<sc::GaussianBasisSet> bs3_;
    Ref<sc::GaussianBasisSet> bs4_;
    Ref<sc::Integral> integral_;
    Ref<sc::TwoBodyInt> eval_;
    Ref<sc::TwoBodyDerivInt> deriv_eval_;
    int max_nshell4_;
    sidl::array<double> sidl_buffer_;
    const double* sc_buffer_;
    enum { two_body, two_body_deriv};
    int int_type_;
    int deriv_level_;
    std::string package_;
    // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator4._implementation)

  private:
    // private default constructor (required)
    IntegralEvaluator4_impl() {} 

  public:
    // sidl constructor (required)
    // Note: alternate Skel constructor doesn't call addref()
    // (fixes bug #275)
    IntegralEvaluator4_impl( struct MPQC_IntegralEvaluator4__object * s ) : 
      self(s,true) { _ctor(); }

    // user defined construction
    void _ctor();

    // virtual destructor (required)
    virtual ~IntegralEvaluator4_impl() { _dtor(); }

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
     * @param bs4 Molecular basis on center 4.
     * @param label String specifying integral type.
     * @param max_deriv Max derivative to compute. 
     */
    void
    initialize (
      /*in*/ ::Chemistry::QC::GaussianBasis::Molecular bs1,
      /*in*/ ::Chemistry::QC::GaussianBasis::Molecular bs2,
      /*in*/ ::Chemistry::QC::GaussianBasis::Molecular bs3,
      /*in*/ ::Chemistry::QC::GaussianBasis::Molecular bs4,
      /*in*/ const ::std::string& label,
      /*in*/ int64_t max_deriv
    )
    throw () 
    ;


    /**
     * Get the buffer pointer.
     * @return Buffer pointer. 
     */
    void*
    get_buffer() throw () 
    ;

    /**
     * Compute a shell quartet of integrals.
     * @param shellnum1 Gaussian shell number 1.
     * @param shellnum2 Gaussian shell number 2.
     * @param shellnum3 Gaussian shell number 3.
     * @param shellnum4 Gaussian shell number 4.
     * @param deriv_level Derivative level. 
     */
    void
    compute (
      /*in*/ int64_t shellnum1,
      /*in*/ int64_t shellnum2,
      /*in*/ int64_t shellnum3,
      /*in*/ int64_t shellnum4,
      /*in*/ int64_t deriv_level
    )
    throw () 
    ;


    /**
     * Compute a shell quartet of integrals and return as a borrowed
     * sidl array.
     * @param shellnum1 Gaussian shell number 1.
     * @param shellnum2 Gaussian shell number 2.
     * @param shellnum3 Guassian shell number 3.
     * @param shellnum4 Gaussian shell number 4.
     * @param deriv_level Derivative level.
     * @return Borrowed sidl array buffer. 
     */
    ::sidl::array<double>
    compute_array (
      /*in*/ int64_t shellnum1,
      /*in*/ int64_t shellnum2,
      /*in*/ int64_t shellnum3,
      /*in*/ int64_t shellnum4,
      /*in*/ int64_t deriv_level
    )
    throw () 
    ;

  };  // end class IntegralEvaluator4_impl

} // end namespace MPQC

// DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator4._misc)
// Put miscellaneous things here...
// DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator4._misc)

#endif
