//
// formula.h --- class for calculation molecular formulae
//
// Copyright (C) 1997 Limit Point Systems, Inc.
//
// Author: Edward Seidl <seidl@janed.com>
// Maintainer: LPS
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

#ifndef _chemistry_molecule_formula_h
#define _chemistry_molecule_formula_h

#ifdef __GNUC__
#pragma interface
#endif

#include <chemistry/molecule/molecule.h>

class MolecularFormula {
  private:
    const int nelem_ = 105;
    int count_[nelem_];
    char *form_;
    
  public:
    MolecularFormula(const RefMolecule&);
    ~MolecularFormula();

    const char * formula() const;
};

#endif
