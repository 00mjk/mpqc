// 
// File:          MPQC_IntegralEvaluator2_Impl.cc
// Symbol:        MPQC.IntegralEvaluator2-v0.2
// Symbol Type:   class
// Babel Version: 0.10.12
// Description:   Server-side implementation for MPQC.IntegralEvaluator2
// 
// WARNING: Automatically generated; only changes within splicers preserved
// 
// babel-version = 0.10.12
// 
#include "MPQC_IntegralEvaluator2_Impl.hh"

// DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2._includes)

sc::Ref<sc::GaussianBasisSet>
basis_cca_to_sc( Chemistry::QC::GaussianBasis::Molecular& );

// DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2._includes)

// user-defined constructor.
void MPQC::IntegralEvaluator2_impl::_ctor() {
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2._ctor)
  reorder_ = false;
  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2._ctor)
}

// user-defined destructor.
void MPQC::IntegralEvaluator2_impl::_dtor() {
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2._dtor)
  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2._dtor)
}

// static class initializer.
void MPQC::IntegralEvaluator2_impl::_load() {
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2._load)
  // guaranteed to be called at most once before any other method in this class
  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2._load)
}

// user-defined static methods: (none)

// user-defined non-static methods:
/**
 * Method:  add_evaluator[]
 */
void
MPQC::IntegralEvaluator2_impl::add_evaluator (
  /* in */ void* eval,
  /* in */ ::Chemistry::QC::GaussianBasis::IntegralDescr desc ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.add_evaluator)  

  if( desc.get_deriv_lvl() == 0 )
    eval_.add_evaluator(eval,desc);
  else 
    deriv_eval_.add_evaluator(eval,desc);

  buffer_size_.update( desc.get_deriv_lvl(), desc.get_n_segment() );

  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.add_evaluator)
}

/**
 * Method:  set_basis[]
 */
void
MPQC::IntegralEvaluator2_impl::set_basis (
  /* in */ ::Chemistry::QC::GaussianBasis::Molecular bs1,
  /* in */ ::Chemistry::QC::GaussianBasis::Molecular bs2 ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.set_basis)

  bs1_ = basis_cca_to_sc( bs1 );
  if( bs2.isSame(bs1) )
    bs2_ = bs1_;
  else
    bs2_ = basis_cca_to_sc( bs2 );

  buffer_size_.init( 2, bs1_, bs2_, bs3_, bs4_ );
  
  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.set_basis)
}

/**
 * Method:  init_reorder[]
 */
void
MPQC::IntegralEvaluator2_impl::init_reorder ()
throw () 

{
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.init_reorder)

  reorder_ = true;
  reorder_engine_.init( 2, bs1_, bs2_, bs3_, bs4_ );
  CompositeIntegralDescr desc = eval_.get_descriptor();
  for( int i=0; i < desc.get_n_descr(); ++i) {
    IntegralDescr idesc = desc.get_descr(i);
    reorder_engine_.add_buffer( eval_.get_buffer( idesc ), idesc );
  }
  CompositeIntegralDescr deriv_desc = deriv_eval_.get_descriptor();
  for( int i=0; i < deriv_desc.get_n_descr(); ++i) {
    IntegralDescr idesc = deriv_desc.get_descr(i);
    reorder_engine_.add_buffer( deriv_eval_.get_buffer( idesc ), idesc );
  }

  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.init_reorder)
}

/**
 * Method:  set_opaque_deriv_centers[]
 */
void
MPQC::IntegralEvaluator2_impl::set_opaque_deriv_centers (
  /* in */ void* dc ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.set_opaque_deriv_centers)
  // Insert-Code-Here {MPQC.IntegralEvaluator2.set_opaque_deriv_centers} (set_opaque_deriv_centers method)
  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.set_opaque_deriv_centers)
}

/**
 * Get buffer pointer for given type.
 * @return Buffer pointer. 
 */
void*
MPQC::IntegralEvaluator2_impl::get_buffer (
  /* in */ ::Chemistry::QC::GaussianBasis::IntegralDescr desc ) 
throw ( 
  ::sidl::BaseException
){
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.get_buffer)

  if( desc.get_deriv_lvl() == 0 )
    return eval_.get_buffer( desc );
  else
    return deriv_eval_.get_buffer( desc );

  return NULL;
  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.get_buffer)
}

/**
 * Method:  get_descriptor[]
 */
::Chemistry::QC::GaussianBasis::CompositeIntegralDescr
MPQC::IntegralEvaluator2_impl::get_descriptor ()
throw ( 
  ::sidl::BaseException
)
{
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.get_descriptor)

  CompositeIntegralDescr cdesc = Chemistry::CompositeIntegralDescr::_create();
  CompositeIntegralDescr desc =  eval_.get_descriptor();
  CompositeIntegralDescr deriv_desc = deriv_eval_.get_descriptor();
  for( int i=0; i<desc.get_n_descr(); ++i)
    cdesc.add_descr( desc.get_descr(i) );
  for( int i=0; i<deriv_desc.get_n_descr(); ++i)
    cdesc.add_descr( deriv_desc.get_descr(i) );

  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.get_descriptor)
}

/**
 * Compute a shell doublet of integrals.
 * @param shellnum1 Gaussian shell number 1.
 * @param shellnum2 Gaussian shell number 2.
 * @param deriv_level Derivative level. 
 */
void
MPQC::IntegralEvaluator2_impl::compute (
  /* in */ int64_t shellnum1,
  /* in */ int64_t shellnum2 ) 
throw ( 
  ::sidl::BaseException
){
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.compute)
  
  computer_.set_shells( shellnum1, shellnum2 );
  deriv_computer_.set_shells( shellnum1, shellnum2 );
  eval_.compute( &computer_ );
  deriv_eval_.compute( &deriv_computer_ );
  if( reorder_ )
    reorder_engine_.do_it( shellnum1, shellnum2, -1, -1 ); // -1 ignored

  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.compute)
}

/**
 * Compute a shell doublet of integrals and return as a borrowed
 * sidl array.
 * @param shellnum1 Gaussian shell number 1.
 * @param shellnum2 Gaussian shell number 2.
 * @return Borrowed sidl array buffer. 
 */
::sidl::array<double>
MPQC::IntegralEvaluator2_impl::compute_array (
  /* in */ const ::std::string& type,
  /* in */ int32_t deriv_lvl,
  /* in */ int64_t shellnum1,
  /* in */ int64_t shellnum2 ) 
throw ( 
  ::sidl::BaseException
){
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.compute_array)

  sidl::array<double> array;
  if( deriv_lvl == 0 ) {
    computer_.set_shells( shellnum1, shellnum2 );
    array = eval_.compute_array( &computer_, type, 
                                 deriv_lvl, buffer_size_.size() );
  }
  else {
    deriv_computer_.set_shells( shellnum1, shellnum2 );
    array = deriv_eval_.compute_array( &deriv_computer_, type, 
                                       deriv_lvl, buffer_size_.size() ); 
  }
  if( reorder_ )
    reorder_engine_.do_it( shellnum1, shellnum2, -1, -1 );

  return array;

  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.compute_array)
}

/**
 * Compute integral bounds.
 * @param shellnum1 Gaussian shell number 1.
 * @param shellnum2 Gaussian shell number 2. 
 */
double
MPQC::IntegralEvaluator2_impl::compute_bounds (
  /* in */ int64_t shellnum1,
  /* in */ int64_t shellnum2 ) 
throw ( 
  ::sidl::BaseException
){
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.compute_bounds)

  sidl::SIDLException ex = sidl::SIDLException::_create();
  try {
    ex.setNote("function not implemented yet");
    ex.add(__FILE__, __LINE__,"");
  }
  catch(...) { }
  throw ex;

  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.compute_bounds)
}

/**
 * Compute array of integral bounds.
 * @param shellnum1 Gaussian shell number 1.
 * @param shellnum2 Gaussian shell number 2. 
 */
::sidl::array<double>
MPQC::IntegralEvaluator2_impl::compute_bounds_array (
  /* in */ int64_t shellnum1,
  /* in */ int64_t shellnum2 ) 
throw ( 
  ::sidl::BaseException
){
  // DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2.compute_bounds_array)

  sidl::SIDLException ex = sidl::SIDLException::_create();
  try {
    ex.setNote("function not implemented yet");
    ex.add(__FILE__, __LINE__,"");
  }
  catch(...) { }
  throw ex;

  // DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2.compute_bounds_array)
}


// DO-NOT-DELETE splicer.begin(MPQC.IntegralEvaluator2._misc)
// DO-NOT-DELETE splicer.end(MPQC.IntegralEvaluator2._misc)
