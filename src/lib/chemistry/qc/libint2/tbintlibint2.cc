//
// tbintlibint2.cc
//
// Copyright (C) 2001 Edward Valeev
//
// Author: Edward Valeev <evaleev@vt.edu>
// Maintainer: EV
//
// This file is part of the SC Toolkit.
//
// The SC Toolkit is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SC Toolkit is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SC Toolkit; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
// The U.S. Government is granted a limited license as per AL 91-7.
//

#include <libint2/libint2.h>

#include <util/class/class.h>
#include <util/class/scexception.h>
#include <chemistry/qc/basis/integral.h>
#include <chemistry/qc/basis/intdescr.h>
#include <chemistry/qc/libint2/tbintlibint2.h>
#include <chemistry/qc/libint2/bounds.h>
#include <chemistry/qc/libint2/bounds.timpl.h>
#if LIBINT2_SUPPORT_ERI
#  include <chemistry/qc/libint2/eri.h>
#  include <chemistry/qc/libint2/g12nc.h>
#endif
#if LIBINT2_SUPPORT_G12
# if LIBINT2_SUPPORT_T1G12
#  include <chemistry/qc/libint2/g12.h>
# endif
#endif
#if LIBINT2_SUPPORT_G12DKH
#  include <chemistry/qc/libint2/g12dkh.h>
#endif

using namespace std;
using namespace sc;

namespace util {
    template <class To, class From>
    Ref<To> require_dynamic_cast(const Ref<From>& fptr) {
	To* tptr = dynamic_cast<To*>(fptr.pointer());
	if (!tptr)
	    throw ProgrammingError("Dynamic cast failed",__FILE__,__LINE__);
	return tptr;
    }
}

//////////////////////////////////////////////////////////////////////////

namespace sc {
    namespace libint2 {

#if LIBINT2_SUPPORT_ERI
	template<>
	Ref<EriLibint2>
	create_int2e(Integral*integral,
		     const Ref<GaussianBasisSet>& b1,
		     const Ref<GaussianBasisSet>& b2,
		     const Ref<GaussianBasisSet>& b3,
		     const Ref<GaussianBasisSet>& b4,
		     size_t storage,
		     const Ref<IntParams>& params)
	{
	    return new EriLibint2(integral,b1,b2,b3,b4,storage);
	}
    template<>
    Ref<G12NCLibint2>
    create_int2e(Integral*integral,
             const Ref<GaussianBasisSet>& b1,
             const Ref<GaussianBasisSet>& b2,
             const Ref<GaussianBasisSet>& b3,
             const Ref<GaussianBasisSet>& b4,
             size_t storage,
             const Ref<IntParams>& params)
    {
        typedef IntParamsG12 IPType;
        Ref<IPType> params_cast = util::require_dynamic_cast<IPType,IntParams>(params);
        return new G12NCLibint2(integral,b1,b2,b3,b4,storage,params_cast->bra(),params_cast->ket());
    }
#endif

#if LIBINT2_SUPPORT_G12
# if LIBINT2_SUPPORT_T1G12
	template<>
	Ref<G12Libint2>
	create_int2e(Integral*integral,
		     const Ref<GaussianBasisSet>& b1,
		     const Ref<GaussianBasisSet>& b2,
		     const Ref<GaussianBasisSet>& b3,
		     const Ref<GaussianBasisSet>& b4,
		     size_t storage,
		     const Ref<IntParams>& params)
	{
	    typedef IntParamsG12 IPType;
	    Ref<IPType> params_cast = util::require_dynamic_cast<IPType,IntParams>(params);
	    return new G12Libint2(integral,b1,b2,b3,b4,storage,params_cast->bra(),params_cast->ket());
	}
# endif
#endif
#if LIBINT2_SUPPORT_G12DKH
	   template<>
	    Ref<G12DKHLibint2>
	    create_int2e(Integral*integral,
	             const Ref<GaussianBasisSet>& b1,
	             const Ref<GaussianBasisSet>& b2,
	             const Ref<GaussianBasisSet>& b3,
	             const Ref<GaussianBasisSet>& b4,
	             size_t storage,
	             const Ref<IntParams>& params)
	    {
	        typedef IntParamsG12 IPType;
	        Ref<IPType> params_cast = util::require_dynamic_cast<IPType,IntParams>(params);
	        if (params_cast->bra() != params_cast->ket())
	          throw FeatureNotImplemented("G12DKH integrals are currently available for 1 correlation factor only",__FILE__,__LINE__);
	        return new G12DKHLibint2(integral,b1,b2,b3,b4,storage,params_cast->bra());
	    }
#endif
    }
}

TwoBodyIntLibint2::TwoBodyIntLibint2(Integral*integral,
				     const Ref<GaussianBasisSet>& b1,
				     const Ref<GaussianBasisSet>& b2,
				     const Ref<GaussianBasisSet>& b3,
				     const Ref<GaussianBasisSet>& b4,
				     size_t storage, TwoBodyOperSet::type int2etype,
				     const Ref<IntParams>& params):
    TwoBodyInt(integral,b1,b2,b3,b4), int2etype_(int2etype),
    descr_(TwoBodyOperSetDescr::instance(int2etype)),
    params_(params)
{
    using sc::libint2::create_int2e;
  // Which evaluator to use
  switch (int2etype_) {
#if LIBINT2_SUPPORT_ERI
  case TwoBodyOperSet::ERI:
  {
    typedef EriLibint2 Int2e;
    typedef BoundsLibint2<Int2e> Bounds;
    Ref<Bounds> bounds = new Bounds(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_ = create_int2e<EriLibint2>(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_->bounds(bounds);
    break;
  }
  case TwoBodyOperSet::G12NC:
  {
    typedef G12NCLibint2 Int2e;
    typedef BoundsLibint2<Int2e> Bounds;
    Ref<Bounds> bounds = new Bounds(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_ = create_int2e<G12NCLibint2>(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_->bounds(bounds);
    break;
  }
#endif
#if LIBINT2_SUPPORT_G12
# if LIBINT2_SUPPORT_T1G12
  case TwoBodyOperSet::G12:
  {
    int2elibint2_ = create_int2e<G12Libint2>(integral,b1,b2,b3,b4,storage,params);
    break;
  }
# endif
#endif

#if LIBINT2_SUPPORT_G12DKH
  case TwoBodyOperSet::G12DKH:
  {
    int2elibint2_ = create_int2e<G12DKHLibint2>(integral,b1,b2,b3,b4,storage,params);
    break;
  }
#endif
  default:
    throw FeatureNotImplemented("Tried to construct a two-electron integral \
evaluator of unimplemented or unknown type",__FILE__,__LINE__);
  }

  buffer_ = int2elibint2_->buffer(TwoBodyOper::eri);
  integral_->adjust_storage(int2elibint2_->storage_used());
}

TwoBodyIntLibint2::~TwoBodyIntLibint2()
{
  integral_->adjust_storage(-int2elibint2_->storage_used());
}

void
TwoBodyIntLibint2::compute_shell(int is, int js, int ks, int ls)
{
  int2elibint2_->set_redundant(redundant());
  int2elibint2_->compute_quartet(&is,&js,&ks,&ls);
}

int
TwoBodyIntLibint2::log2_shell_bound(int is, int js, int ks, int ls)
{
  const int bound = int2elibint2_->log2_bound(is,js,ks,ls);
  return bound;
}

void
TwoBodyIntLibint2::set_integral_storage(size_t storage)
{
  int2elibint2_->init_storage(storage);
}

//////////////////////////////////////////////////////////////////////////

TwoBodyThreeCenterIntLibint2::TwoBodyThreeCenterIntLibint2(Integral*integral,
                     const Ref<GaussianBasisSet>& b1,
                     const Ref<GaussianBasisSet>& b2,
                     const Ref<GaussianBasisSet>& b3,
                     size_t storage, TwoBodyOperSet::type int2etype,
                     const Ref<IntParams>& params):
    TwoBodyThreeCenterInt(integral,b1,b2,b3), int2etype_(int2etype),
    descr_(TwoBodyOperSetDescr::instance(int2etype)),
    params_(params)
{
  Ref<GaussianBasisSet> b4 = new GaussianBasisSet(GaussianBasisSet::Unit);
  using sc::libint2::create_int2e;
  // Which evaluator to use
  switch (int2etype_) {
#if LIBINT2_SUPPORT_ERI
  case TwoBodyOperSet::ERI:
  {
    typedef EriLibint2 Int2e;
    typedef BoundsLibint2<Int2e> Bounds;
    Ref<Bounds> bounds = new Bounds(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_ = create_int2e<EriLibint2>(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_->bounds(bounds);
    break;
  }
  case TwoBodyOperSet::G12NC:
  {
    typedef G12NCLibint2 Int2e;
    typedef BoundsLibint2<Int2e> Bounds;
    Ref<Bounds> bounds = new Bounds(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_ = create_int2e<G12NCLibint2>(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_->bounds(bounds);
    break;
  }
#endif
#if LIBINT2_SUPPORT_G12
# if LIBINT2_SUPPORT_T1G12
  case TwoBodyOperSet::G12:
  {
    int2elibint2_ = create_int2e<G12Libint2>(integral,b1,b2,b3,b4,storage,params);
    break;
  }
# endif
#endif

#if LIBINT2_SUPPORT_G12DKH
  case TwoBodyOperSet::G12DKH:
  {
    int2elibint2_ = create_int2e<G12DKHLibint2>(integral,b1,b2,b3,b4,storage,params);
    break;
  }
#endif
  default:
    throw FeatureNotImplemented("Tried to construct a two-electron integral \
evaluator of unimplemented or unknown type",__FILE__,__LINE__);
  }

  buffer_ = int2elibint2_->buffer(TwoBodyOper::eri);
  integral_->adjust_storage(int2elibint2_->storage_used());
}

TwoBodyThreeCenterIntLibint2::~TwoBodyThreeCenterIntLibint2()
{
  integral_->adjust_storage(-int2elibint2_->storage_used());
}

void
TwoBodyThreeCenterIntLibint2::compute_shell(int is, int js, int ks)
{
  int ls = 0;
  int2elibint2_->set_redundant(redundant());
  int2elibint2_->compute_quartet(&is,&js,&ks,&ls);
}

int
TwoBodyThreeCenterIntLibint2::log2_shell_bound(int is, int js, int ks)
{
  const int bound = int2elibint2_->log2_bound(is,js,ks,0);
  return bound;
}

void
TwoBodyThreeCenterIntLibint2::set_integral_storage(size_t storage)
{
  int2elibint2_->init_storage(storage);
}
//////////////////////////////////////////////////////////////////////////

TwoBodyTwoCenterIntLibint2::TwoBodyTwoCenterIntLibint2(Integral*integral,
                     const Ref<GaussianBasisSet>& b1,
                     const Ref<GaussianBasisSet>& b3,
                     size_t storage, TwoBodyOperSet::type int2etype,
                     const Ref<IntParams>& params):
    TwoBodyTwoCenterInt(integral,b1,b3), int2etype_(int2etype),
    descr_(TwoBodyOperSetDescr::instance(int2etype)),
    params_(params)
{
  Ref<GaussianBasisSet> b2 = new GaussianBasisSet(GaussianBasisSet::Unit);
  Ref<GaussianBasisSet> b4 = b2;
  using sc::libint2::create_int2e;
  // Which evaluator to use
  switch (int2etype_) {
#if LIBINT2_SUPPORT_ERI
  case TwoBodyOperSet::ERI:
  {
    typedef EriLibint2 Int2e;
    typedef BoundsLibint2<Int2e> Bounds;
    Ref<Bounds> bounds = new Bounds(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_ = create_int2e<EriLibint2>(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_->bounds(bounds);
    break;
  }
  case TwoBodyOperSet::G12NC:
  {
    typedef G12NCLibint2 Int2e;
    typedef BoundsLibint2<Int2e> Bounds;
    Ref<Bounds> bounds = new Bounds(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_ = create_int2e<G12NCLibint2>(integral,b1,b2,b3,b4,storage,params);
    int2elibint2_->bounds(bounds);
    break;
  }
#endif
#if LIBINT2_SUPPORT_G12
# if LIBINT2_SUPPORT_T1G12
  case TwoBodyOperSet::G12:
  {
    int2elibint2_ = create_int2e<G12Libint2>(integral,b1,b2,b3,b4,storage,params);
    break;
  }
# endif
#endif

#if LIBINT2_SUPPORT_G12DKH
  case TwoBodyOperSet::G12DKH:
  {
    int2elibint2_ = create_int2e<G12DKHLibint2>(integral,b1,b2,b3,b4,storage,params);
    break;
  }
#endif
  default:
    throw FeatureNotImplemented("Tried to construct a two-electron integral \
evaluator of unimplemented or unknown type",__FILE__,__LINE__);
  }

  buffer_ = int2elibint2_->buffer(TwoBodyOper::eri);
  integral_->adjust_storage(int2elibint2_->storage_used());
}

TwoBodyTwoCenterIntLibint2::~TwoBodyTwoCenterIntLibint2()
{
  integral_->adjust_storage(-int2elibint2_->storage_used());
}

void
TwoBodyTwoCenterIntLibint2::compute_shell(int is, int ks)
{
  int js = 0;
  int ls = 0;
  int2elibint2_->set_redundant(redundant());
  int2elibint2_->compute_quartet(&is,&js,&ks,&ls);
}

int
TwoBodyTwoCenterIntLibint2::log2_shell_bound(int is, int ks)
{
  const int bound = int2elibint2_->log2_bound(is,0,ks,0);
  return bound;
}

void
TwoBodyTwoCenterIntLibint2::set_integral_storage(size_t storage)
{
  int2elibint2_->init_storage(storage);
}

bool
TwoBodyTwoCenterIntLibint2::cloneable()
{
  return true;
}

Ref<TwoBodyTwoCenterInt>
TwoBodyTwoCenterIntLibint2::clone()
{
  const size_t storage = int2elibint2_->storage_used();
  return new TwoBodyTwoCenterIntLibint2(integral_, bs1_, bs2_, storage, int2etype_, params_);
}


//////////////////////////////////////////////////////////////////

TwoBodyDerivIntLibint2::TwoBodyDerivIntLibint2(Integral*integral,
					   const Ref<GaussianBasisSet>& b1,
					   const Ref<GaussianBasisSet>& b2,
					   const Ref<GaussianBasisSet>& b3,
					   const Ref<GaussianBasisSet>& b4,
					   size_t storage, TwoBodyOperSet::type int2etype):
  TwoBodyDerivInt(integral,b1,b2,b3,b4)
{
  // Which evaluator to use
  switch (int2etype) {
  default:
      throw ProgrammingError("TwoBodyDerivIntLibint2: tried to construct a two-electron derivative integral evaluator of unimplemented or unknown type",__FILE__,__LINE__);
  }

  //  int2elibint2_ = new EriLibint2(integral,b1,b2,b3,b4,1,storage);
  //buffer_ = int2elibint2_->buffer();
  //integral_->adjust_storage(int2elibint2_->storage_used());
}

TwoBodyDerivIntLibint2::~TwoBodyDerivIntLibint2()
{
  //  integral_->adjust_storage(-int2elibint2_->used_storage());
}

void
TwoBodyDerivIntLibint2::compute_shell(int is, int js, int ks, int ls,
                                 DerivCenters&c)
{
  int center;
  int sh[4], sz[4];

  sh[0]=is; sh[1]=js; sh[2]=ks; sh[3]=ls;

}


int
TwoBodyDerivIntLibint2::log2_shell_bound(int is, int js, int ks, int ls)
{
  return int2elibint2_->log2_bound(is,js,ks,ls);
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
