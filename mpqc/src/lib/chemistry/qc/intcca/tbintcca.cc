//
// tbintcca.cc
//
// Copyright (C) 2004 Sandia National Laboratories
//
// Author: Joe Kenny <jpkenny@sandia.gov>
// Maintainer: Joe Kenny
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
#include <chemistry/qc/intcca/tbintcca.h>

using namespace Chemistry::QC::GaussianBasis;
using namespace sc;

////////////////////////////////////////////////////////////////////////////
// TwoBodyIntCCA

TwoBodyIntCCA::TwoBodyIntCCA(Integral* integral,
                             const Ref<GaussianBasisSet> &bs1,
                             const Ref<GaussianBasisSet> &bs2,
			     const Ref<GaussianBasisSet> &bs3,
			     const Ref<GaussianBasisSet> &bs4,
			     size_t storage,
			     IntegralEvaluatorFactory eval_factory, 
                             bool use_opaque) :
  TwoBodyInt(integral,bs1,bs2,bs3,bs4) 
{
  int2ecca_ = new Int2eCCA(integral,bs1,bs2,bs3,bs4,0,storage,
                           eval_factory,use_opaque);
  buffer_ = int2ecca_->buffer();
  int2ecca_->set_redundant(redundant_);
}

TwoBodyIntCCA::~TwoBodyIntCCA()
{
}

void
TwoBodyIntCCA::compute_shell(int is, int js, int ks, int ls)
{
  int2ecca_->compute_erep(is,js,ks,ls);
}

int
TwoBodyIntCCA::log2_shell_bound(int is, int js, int ks, int ls)
{
}

void
TwoBodyIntCCA::set_integral_storage(size_t storage)
{
}

/////////////////////////////////////////////////////////////////////////////
// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
