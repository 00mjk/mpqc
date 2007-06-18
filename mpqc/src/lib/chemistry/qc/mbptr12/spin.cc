//
// spin.cc
//
// Copyright (C) 2005 Edward Valeev
//
// Author: Edward Valeev <edward.valeev@chemistry.gatech.edu>
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

#ifdef __GNUC__
#pragma implementation
#endif

#include <chemistry/qc/mbptr12/spin.h>

namespace sc {
  unsigned int nspincases1(bool spin_polarized) { return spin_polarized ? 2 : 1; }
  unsigned int nspincases2(bool spin_polarized) { return spin_polarized ? 3 : 1; }
  SpinCase1 case1(SpinCase2 S) { return S==BetaBeta ? Beta : Alpha; }
  SpinCase1 case2(SpinCase2 S) { return S==AlphaAlpha ? Alpha : Beta; }
  
  std::string prepend_spincase(SpinCase2 S, const std::string& R, bool lowercase)
  {
    std::string prefix;
    if (S == AlphaAlpha)
      prefix = (lowercase ? "alpha-alpha " : "Alpha-alpha ");
    else if (S == AlphaBeta)
      prefix = (lowercase ? "alpha-beta " : "Alpha-beta ");
    else
      prefix = (lowercase ? "beta-beta " : "Beta-beta ");
    return prefix + R;
  }
  
  std::string prepend_spincase(PureSpinCase2 S, const std::string& R, bool lowercase)
  {
    std::string prefix;
    if (S == Singlet)
      prefix = (lowercase ? "singlet " : "Singlet ");
    else
      prefix = (lowercase ? "triplet " : "Triplet ");
    return prefix + R;
  }
  
  std::string prepend_spincase(SpinCase1 S, const std::string& R, bool lowercase)
  {
    std::string prefix;
    if (S == Alpha)
      prefix = (lowercase ? "alpha " : "Alpha ");
    else
      prefix = (lowercase ? "beta " : "Beta ");
    return prefix + R;
  }
  
}
