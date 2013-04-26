//
// operator.cc
//
// Copyright (C) 2007 Edward Valeev
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

#include <cassert>
#include <util/misc/scexception.h>
#include <chemistry/qc/basis/operator.h>

using namespace sc;

////

int OneBodyOper::max_ntypes = static_cast<int>(OneBodyOper::invalid);

Ref<OneBodyOperDescr>
OneBodyOper::descr(OneBodyOper::type t) {
  static Ref<OneBodyOperDescr> hermitian = new OneBodyOperDescr(+1);
  static Ref<OneBodyOperDescr> antihermitian = new OneBodyOperDescr(-1);
  switch(t) {
    case Nabla_x:
    case Nabla_y:
    case Nabla_z:
    case iL_x:
    case iL_y:
    case iL_z:
      return antihermitian;

    case T:
    case V:
    case h:
    case J:
    case K:
    case F:
    case mu_x:
    case mu_y:
    case mu_z:
    case q_xx:
    case q_xy:
    case q_xz:
    case q_yy:
    case q_yz:
    case q_zz:
    case p4:
    case pVp:
    case pxVp_x:
    case pxVp_y:
    case pxVp_z:
      return hermitian;

    default:
      throw ProgrammingError("TwoBodyOper::descr() -- incorrect type");
  }

  return Ref<OneBodyOperDescr>(); // shut up stupid compiler
}

enum type {
  T = 0,      //!< (nonrelativitic) kinetic energy
  V = 1,      //!< nuclear (Coulomb) potential
  h = 2,      //!< core Hamiltonian = T+V
  J = 3,      //!< (electronic) Coulomb
  K = 4,      //!< (electronic) exchange
  F = 5,      //!< Fock operator
  mu_x = 6,   //!< x component of electric dipole moment
  mu_y = 7,   //!< y component of electric dipole moment
  mu_z = 8,   //!< z component of electric dipole moment
  q_xx = 9,   //!< xx component of quadrupole moment
  q_xy = 10,  //!< xy component of quadrupole moment
  q_xz = 11,  //!< xz component of quadrupole moment
  q_yy = 12,  //!< yy component of quadrupole moment
  q_yz = 13,  //!< yz component of quadrupole moment
  q_zz = 14,  //!< zz component of quadrupole moment
  pVp = 15,   //!< \$f \underline{\hat{p}} \cdot V \underline{\hat{p}} \f$
  pxVp_x = 16,//!< x component of \$f \underline{\hat{p}} \cross V \underline{\hat{p}} \f$
  pxVp_y = 17,//!< y component of \$f \underline{\hat{p}} \cross V \underline{\hat{p}} \f$
  pxVp_z = 18,//!< z component of \$f \underline{\hat{p}} \cross V \underline{\hat{p}} \f$
  p4 = 19,      //!< \f$ (\underline{\hat{p}} \cdot \underline{\hat{p}})^2 \f$
  Nabla_x = 20,   //!< x component of Nabla operator ( \f$ \equiv i \hat{p}_x \f$ )
  Nabla_y = 21,   //!< y component of Nabla operator ( \f$ \equiv i \hat{p}_y \f$ )
  Nabla_z = 22,   //!< z component of Nabla operator ( \f$ \equiv i \hat{p}_z \f$ )
  iL_x = 23,   //!< x component of negative imaginary part of angular momentum ( \f$ \equiv i \hat{L}_x \f$ )
  iL_y = 24,   //!< y component of negative imaginary part of angular momentum ( \f$ \equiv i \hat{L}_y \f$ )
  iL_z = 25,    //!< z component of negative imaginary part of angular momentum ( \f$ \equiv i \hat{L}_z \f$ )
  invalid = 26
};

std::string
OneBodyOper::to_string(OneBodyOper::type t) {
  switch (t) {
    case T: return "T"; break;
    case V: return "V"; break;
    case h: return "h"; break;
    case J: return "J"; break;
    case K: return "K"; break;
    case F: return "F"; break;
    case hJ: return "hJ"; break;
    case mu_x: return "mu_x"; break;
    case mu_y: return "mu_y"; break;
    case mu_z: return "mu_z"; break;
    case q_xx: return "q_xx"; break;
    case q_xy: return "q_xy"; break;
    case q_xz: return "q_xz"; break;
    case q_yy: return "q_yy"; break;
    case q_yz: return "q_yz"; break;
    case q_zz: return "q_zz"; break;
    case pVp: return "pVp"; break;
    case pxVp_x: return "pxVp_x"; break;
    case pxVp_y: return "pxVp_y"; break;
    case pxVp_z: return "pxVp_z"; break;
    case p4: return "p4"; break;
    case Nabla_x: return "Nabla_x"; break;
    case Nabla_y: return "Nabla_y"; break;
    case Nabla_z: return "Nabla_z"; break;
    case iL_x: return "iL_x"; break;
    case iL_y: return "iL_y"; break;
    case iL_z: return "iL_z"; break;
    default:
      std::ostringstream oss;
      oss << "OneBodyOper::to_string: unknown type " << t;
      throw ProgrammingError(oss.str().c_str(), __FILE__, __LINE__);
  }
}

OneBodyOperDescr::OneBodyOperDescr(int perm) : perm_(perm) {
}

unsigned int
OneBodyOperDescr::num_particles() const {
  return 1;
}

int
OneBodyOperDescr::perm_symm(unsigned int i) const {
  assert(i == 0);
  return perm_;
}

int
OneBodyOperDescr::perm_symm(unsigned int i, unsigned int j) const {
  throw ProgrammingError("OneBodyOperDescr::perm_symm(i,j) -- lacks meaning", __FILE__, __LINE__);
}

OneBodyOper::type OneBodyOperSetTypeMap<OneBodyOperSet::T>::value[] = {OneBodyOper::T};
OneBodyOper::type OneBodyOperSetTypeMap<OneBodyOperSet::V>::value[] = {OneBodyOper::V};
OneBodyOper::type OneBodyOperSetTypeMap<OneBodyOperSet::h>::value[] = {OneBodyOper::h};
OneBodyOper::type OneBodyOperSetTypeMap<OneBodyOperSet::mu>::value[] = {OneBodyOper::mu_x,
                                                                 OneBodyOper::mu_y,
                                                                 OneBodyOper::mu_z};
OneBodyOper::type OneBodyOperSetTypeMap<OneBodyOperSet::q>::value[] = {OneBodyOper::q_xx,
                                                                OneBodyOper::q_xy,
                                                                OneBodyOper::q_xz,
                                                                OneBodyOper::q_yy,
                                                                OneBodyOper::q_yz,
                                                                OneBodyOper::q_zz};
OneBodyOper::type OneBodyOperSetTypeMap<OneBodyOperSet::pVp>::value[] = {OneBodyOper::pVp};
OneBodyOper::type OneBodyOperSetTypeMap<OneBodyOperSet::p4>::value[] = {OneBodyOper::p4};

Ref<OneBodyOperSetDescr>
OneBodyOperSetDescr::instance(OneBodyOperSet::type oset)
{
  switch (oset) {
    case OneBodyOperSet::T:
      return new OneBodyOperSetDescr(OneBodyOperSetTypeMap<OneBodyOperSet::T>::size,
                                     OneBodyOperSetTypeMap<OneBodyOperSet::T>::value);
      break;
    case OneBodyOperSet::V:
      return new OneBodyOperSetDescr(OneBodyOperSetTypeMap<OneBodyOperSet::V>::size,
                                     OneBodyOperSetTypeMap<OneBodyOperSet::V>::value);
      break;
    case OneBodyOperSet::h:
      return new OneBodyOperSetDescr(OneBodyOperSetTypeMap<OneBodyOperSet::h>::size,
                                     OneBodyOperSetTypeMap<OneBodyOperSet::h>::value);
      break;
    case OneBodyOperSet::mu:
      return new OneBodyOperSetDescr(OneBodyOperSetTypeMap<OneBodyOperSet::mu>::size,
                                     OneBodyOperSetTypeMap<OneBodyOperSet::mu>::value);
      break;
    case OneBodyOperSet::q:
      return new OneBodyOperSetDescr(OneBodyOperSetTypeMap<OneBodyOperSet::q>::size,
                                     OneBodyOperSetTypeMap<OneBodyOperSet::q>::value);
      break;
    case OneBodyOperSet::pVp:
      return new OneBodyOperSetDescr(OneBodyOperSetTypeMap<OneBodyOperSet::pVp>::size,
                                     OneBodyOperSetTypeMap<OneBodyOperSet::pVp>::value);
      break;
    case OneBodyOperSet::p4:
      return new OneBodyOperSetDescr(OneBodyOperSetTypeMap<OneBodyOperSet::p4>::size,
                                     OneBodyOperSetTypeMap<OneBodyOperSet::p4>::value);
      break;
    default:
      assert(false);
  }
  return Ref<OneBodyOperSetDescr>(); // unreachable
}

OneBodyOper::type
OneBodyOperSetDescr::opertype(unsigned int o) const
{
  assert(o < size_);
  return value_[o];
}

unsigned int
OneBodyOperSetDescr::opertype(OneBodyOper::type o) const
{
  for(int i=0; i<size_; ++i)
    if (o == value_[i])
      return i;
  abort();  // should be unreachable
}

////

int TwoBodyOper::max_ntypes = DistArray4::max_num_te_types;

TwoBodyOperDescr::TwoBodyOperDescr(int perm_p1, int perm_p2, int perm_p12) :
  perm_p1_(perm_p1), perm_p2_(perm_p2), perm_p12_(perm_p12)
{
}

unsigned int TwoBodyOperDescr::num_particles() const { return 2; }
int TwoBodyOperDescr::perm_symm(unsigned int i) const {
  if (i == 1) return perm_p1_;
  if (i == 2) return perm_p2_;
  throw ProgrammingError("IntBodyIntTypeDescr::perm_symm(i) -- i must be 1 or 2",__FILE__,__LINE__);
}
int TwoBodyOperDescr::perm_symm(unsigned int i, unsigned int j) const {
  if ( (i == 1 && j == 2) ||
       (i == 2 && j == 1) ) return perm_p12_;
  throw ProgrammingError("IntBodyIntTypeDescr::perm_symm(i,j) -- i,j must be 1,2",__FILE__,__LINE__);
}

Ref<TwoBodyOperDescr>
TwoBodyOper::descr(TwoBodyOper::type type)
{
    static Ref<TwoBodyOperDescr> symm_type = new TwoBodyOperDescr(+1,+1,+1);
    static Ref<TwoBodyOperDescr> t1r12_inttype = new TwoBodyOperDescr(-1,+1,0);
    static Ref<TwoBodyOperDescr> t2r12_inttype = new TwoBodyOperDescr(+1,-1,0);
    switch(type) {
    case TwoBodyOper::r12t1:
    case TwoBodyOper::t1g12:
    return t1r12_inttype;

    case TwoBodyOper::r12t2:
    case TwoBodyOper::t2g12:
    return t2r12_inttype;

    case TwoBodyOper::eri:
    case TwoBodyOper::r12:
    case TwoBodyOper::r12_0_g12:
    case TwoBodyOper::r12_m1_g12:
    case TwoBodyOper::g12t1g12:
    case TwoBodyOper::anti_g12g12:
    case TwoBodyOper::g12p4g12_m_g12t1g12t1:
    return symm_type;
    }
    throw ProgrammingError("TwoBodyOper::descr() -- incorrect type");
}

std::string
TwoBodyOper::to_string(TwoBodyOper::type t) {
  switch (t) {
    case eri: return "g"; break;
    case r12: return "r12"; break;
    case r12t1: return "r12t1"; break;
    case r12t2: return "r12t2"; break;
    case r12_0_g12: return "g12"; break;
    case r12_m1_g12: return "g12g"; break;
    case g12t1g12: return "g12t1g12"; break;
    case t1g12: return "t1g12"; break;
    case t2g12: return "t2g12"; break;
    default:
      std::ostringstream oss;
      oss << "TwoBodyOper::to_string: unknown type " << t;
      throw ProgrammingError(oss.str().c_str(), __FILE__, __LINE__);
  }
}

TwoBodyOper::type TwoBodyOperSetTypeMap<TwoBodyOperSet::ERI>::value[] = {TwoBodyOper::eri};
TwoBodyOper::type TwoBodyOperSetTypeMap<TwoBodyOperSet::R12>::value[] = {TwoBodyOper::eri,
                                                            TwoBodyOper::r12,
                                                            TwoBodyOper::r12t1,
                                                            TwoBodyOper::r12t2};
TwoBodyOper::type TwoBodyOperSetTypeMap<TwoBodyOperSet::G12>::value[] = {TwoBodyOper::eri,
                                                            TwoBodyOper::r12_0_g12,
                                                            TwoBodyOper::r12_m1_g12,
                                                            TwoBodyOper::t1g12,
                                                            TwoBodyOper::t2g12,
                                                            TwoBodyOper::g12t1g12};
TwoBodyOper::type TwoBodyOperSetTypeMap<TwoBodyOperSet::G12NC>::value[] = {TwoBodyOper::eri,
                                                              TwoBodyOper::r12_0_g12,
                                                              TwoBodyOper::r12_m1_g12,
                                                              TwoBodyOper::g12t1g12,
                                                              TwoBodyOper::anti_g12g12};
TwoBodyOper::type TwoBodyOperSetTypeMap<TwoBodyOperSet::G12DKH>::value[] = {TwoBodyOper::g12p4g12_m_g12t1g12t1};
TwoBodyOper::type TwoBodyOperSetTypeMap<TwoBodyOperSet::R12_0_G12>::value[] = {TwoBodyOper::r12_0_g12};
TwoBodyOper::type TwoBodyOperSetTypeMap<TwoBodyOperSet::R12_m1_G12>::value[] = {TwoBodyOper::r12_m1_g12};
TwoBodyOper::type TwoBodyOperSetTypeMap<TwoBodyOperSet::G12_T1_G12>::value[] = {TwoBodyOper::g12t1g12};

TwoBodyOperSetDescr::TwoBodyOperSetDescr(int size,
                                         const TwoBodyOper::type* value) :
  size_(size), value_(value)
  {}

Ref<TwoBodyOperSetDescr>
TwoBodyOperSetDescr::instance(TwoBodyOperSet::type oset)
{
  switch (oset) {
    case TwoBodyOperSet::ERI:
      return new TwoBodyOperSetDescr(TwoBodyOperSetTypeMap<TwoBodyOperSet::ERI>::size,
                                     TwoBodyOperSetTypeMap<TwoBodyOperSet::ERI>::value);
      break;
    case TwoBodyOperSet::G12:
      return new TwoBodyOperSetDescr(TwoBodyOperSetTypeMap<TwoBodyOperSet::G12>::size,
                                     TwoBodyOperSetTypeMap<TwoBodyOperSet::G12>::value);
      break;
    case TwoBodyOperSet::G12NC:
      return new TwoBodyOperSetDescr(TwoBodyOperSetTypeMap<TwoBodyOperSet::G12NC>::size,
                                     TwoBodyOperSetTypeMap<TwoBodyOperSet::G12NC>::value);
      break;
    case TwoBodyOperSet::G12DKH:
      return new TwoBodyOperSetDescr(TwoBodyOperSetTypeMap<TwoBodyOperSet::G12DKH>::size,
                                     TwoBodyOperSetTypeMap<TwoBodyOperSet::G12DKH>::value);
      break;
    case TwoBodyOperSet::R12_0_G12:
      return new TwoBodyOperSetDescr(TwoBodyOperSetTypeMap<TwoBodyOperSet::R12_0_G12>::size,
                                     TwoBodyOperSetTypeMap<TwoBodyOperSet::R12_0_G12>::value);
      break;
    case TwoBodyOperSet::R12_m1_G12:
      return new TwoBodyOperSetDescr(TwoBodyOperSetTypeMap<TwoBodyOperSet::R12_m1_G12>::size,
                                     TwoBodyOperSetTypeMap<TwoBodyOperSet::R12_m1_G12>::value);
      break;
    case TwoBodyOperSet::G12_T1_G12:
      return new TwoBodyOperSetDescr(TwoBodyOperSetTypeMap<TwoBodyOperSet::G12_T1_G12>::size,
                                     TwoBodyOperSetTypeMap<TwoBodyOperSet::G12_T1_G12>::value);
      break;

    default:
      assert(false);
  }
  return Ref<TwoBodyOperSetDescr>(); // unreachable
}

TwoBodyOper::type
TwoBodyOperSetDescr::opertype(unsigned int o) const
{
  assert(o < size_);
  return value_[o];
}

unsigned int
TwoBodyOperSetDescr::opertype(TwoBodyOper::type o) const
{
  for(int i=0; i<size_; ++i)
    if (o == value_[i])
      return i;
  abort();  // should be unreachable
}
