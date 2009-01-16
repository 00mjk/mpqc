//
// fockbuild_runtime.h
//
// Copyright (C) 2009 Edward Valeev
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

#ifdef __GNUG__
#pragma interface
#endif

#ifndef _mpqc_src_lib_chemistry_qc_mbptr12_fockbuildruntime_h
#define _mpqc_src_lib_chemistry_qc_mbptr12_fockbuildruntime_h

#include <util/ref/ref.h>
#include <util/group/thread.h>
#include <util/group/message.h>
#include <math/scmat/matrix.h>
#include <chemistry/qc/basis/basis.h>
#include <chemistry/qc/basis/integral.h>
#include <chemistry/qc/mbptr12/registry.h>
#include <chemistry/qc/mbptr12/spin.h>

namespace sc {

  class FockBuildRuntime : virtual public SavableState {
    public:
      FockBuildRuntime(const Ref<GaussianBasisSet>& refbasis,
                       const RefSymmSCMatrix& aodensity_alpha,
                       const RefSymmSCMatrix& aodensity_beta,
                       const Ref<Integral>& integral,
                       Ref<MessageGrp> msg = MessageGrp::get_default_messagegrp(),
                       Ref<ThreadGrp> thr = ThreadGrp::get_default_threadgrp());
      FockBuildRuntime(StateIn& si);
      void save_data_state(StateOut& so);

      /** Returns true if the given matrix is available
        */
      bool exists(const std::string& key) const;

      /** Returns the matrix corresponding to key.

          key must be in format recognized by ParsedOneBodyIntKey.
          If this key is not known, the matrix will be computed by an appropriate FockMatrixBuild object.
        */
      RefSCMatrix get(const std::string& key);   // non-const: can add transforms

      const Ref<Integral>& integral() const { return integral_; }
      const Ref<MessageGrp>& msg() const { return msg_; }
      const Ref<ThreadGrp>& thr() const { return thr_; }
      const Ref<GaussianBasisSet>& basis() const { return basis_; }

    private:

      Ref<Integral> integral_;
      Ref<MessageGrp> msg_;
      Ref<ThreadGrp> thr_;
      Ref<GaussianBasisSet> basis_;
      bool spin_polarized_;

      // Densities
      RefSymmSCMatrix P_, Po_;

      // Registry of known Fock matrices
      typedef Registry<std::string, RefSCMatrix, detail::NonsingletonCreationPolicy> FockMatrixRegistry;
      Ref<FockMatrixRegistry> registry_;

      /// throws if key is not parsable by ParsedOneBodyIntKey
      void validate_key(const std::string& key) const;

  };

  /// Parsed representation of a string key that represents a set of one-body integrals
  class ParsedOneBodyIntKey {
    public:
      ParsedOneBodyIntKey(const std::string& key);

      const std::string& key() const { return key_; }
      const std::string& bra() const { return bra_; }
      const std::string& ket() const { return ket_; }
      const std::string& oper() const { return oper_; }
      SpinCase1 spin() const { return spin_; }

      /// computes key from its components
      static std::string key(const std::string& bra,
                             const std::string& ket,
                             const std::string& oper,
                             SpinCase1 spin = AnySpinCase1);

    private:
      std::string key_;
      std::string bra_, ket_;
      std::string oper_;
      SpinCase1 spin_;
  };

} // end of namespace sc

#endif // end of header guard


// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End:
