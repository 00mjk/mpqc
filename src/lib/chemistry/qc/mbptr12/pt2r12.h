//
// pt2r12.h
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

#ifndef _mpqc_src_lib_chemistry_qc_mbptr12_pt2r12_h
#define _mpqc_src_lib_chemistry_qc_mbptr12_pt2r12_h

#include <chemistry/qc/wfn/wfn.h>
#include <chemistry/qc/wfn/spin.h>
#include <chemistry/qc/mbptr12/r12wfnworld.h>
#include <chemistry/qc/mbptr12/r12int_eval.h>
#include <chemistry/qc/wfn/rdm.h>

namespace sc {

  /// PT2R12: a universal second-order R12 correction
  class PT2R12 : public Wavefunction {
    public:
      /** A KeyVal constructor is used to generate a PT2R12
          object from the input. This constructor uses keywords of R12WavefunctionWorld,
          plus following list of keywords.

          <table border="1">

          <tr><td>%Keyword<td>Type<td>Default<td>Description

          <tr><td><tt>reference</tt><td>Wavefunction<td>none<td>the Wavefunction object

          <tr><td><tt>rdm2</tt><td>RDM<Two><td>none<td>the RDM<Two> object that provides the 2-RDM. It must
          be constructed from the object provided by the <tt>reference</tt> keyword.

          <tr><td><tt>omit_uocc</tt><td>boolean<td>false<td>if set to true, orbitals not occupied in
          reference will be omitted from consideration. This is useful if only geminal functions
          are used to treat electron correlation.

          <tr><td><tt>cabs_singles</tt><td>boolean<td>false<td>if set to true, compute 2nd-order
          CABS singes correction.
          <tr><td><tt>cabs_singles_coupling</tt><td>boolean<td>true<td>if set to true, include coupling between CABS and OBS virtuals;
              this is the preferred choice and it corresponds to the CABS singles correction without assuming EBC in single reference limit.

          </table>
       */
      PT2R12(const Ref<KeyVal> &keyval);
      PT2R12(StateIn &s);
      ~PT2R12();
      void save_data_state(StateOut &s);

      void compute();
      void print(std::ostream& os =ExEnv::out0()) const;
      RefSymmSCMatrix density();
      int nelectron();
      int spin_polarized();
      int value_implemented() const { return 1; }
      void set_desired_value_accuracy(double acc);

      /// PT2R12 is an R12 Wavefunction
      const Ref<R12WavefunctionWorld>& r12world() const { return r12world_; }

      void obsolete();

    private:

      static double ref_to_pt2r12_acc() { return 0.01; }

      Ref<Wavefunction> reference_;
      Ref< RDM<Two> > rdm2_;
      Ref< RDM<One> > rdm1_;
      Ref<R12IntEval> r12eval_;
      Ref<R12WavefunctionWorld> r12world_;
      unsigned int nfzc_;
      bool omit_uocc_;
      bool pt2_correction_;          // for testing purposes only, set to false to skip the [2]_R12 computation
      bool cabs_singles_;
      bool cabs_singles_coupling_; // if set to true, we include the coupling between cabs and OBS virtual orbitals. This should be preferred choice,
                                   // as explained in the paper.
      bool rotate_core_; // if set to false, when doing rasscf cabs_singles correction, don't include excitation from core orbitals to cabs orbitals in
                         // first-order Hamiltonian. (this may be used when using frozen core orbitals which
                         // are not optimized (does not satisfy Brillouin condition)). Currently, we suggest set it to 'true'
      int debug_;
      std::vector<double> B_;
      std::vector<double> X_;
      std::vector<double> V_; // store the values for different spins


      /// this is to help combine similar functions to one
      enum Tensor4_Permute {Permute23 =1, Permute34 = 2};

      /// 1-RDM as provided by the rdm1_ object
      RefSymmSCMatrix rdm1(SpinCase1 spin);
      /// 2-RDM as provided by the rdm2_ object
      RefSymmSCMatrix rdm2(SpinCase2 spin);
      /// 2-RDM cumulant as provided by the rdm2_->cumulant() object
      RefSymmSCMatrix lambda2(SpinCase2 spin);
      /// gspace block of 1-RDM (@sa R12IntEval::gg_space() )
      RefSymmSCMatrix rdm1_gg(SpinCase1 spin);
      /// gg block of 2-RDM (@sa R12IntEval::gg_space() )
      RefSymmSCMatrix rdm2_gg(SpinCase2 spin);
      /// gg block of 2-RDM cumulant (@sa R12IntEval::gg_space() )
      RefSymmSCMatrix lambda2_gg(SpinCase2 spin);
      // the above 2 functions are implemented using this function
      RefSymmSCMatrix _rdm2_to_gg(SpinCase2 spin,
                                  RefSymmSCMatrix input);

      /// geminal coefficient matrix
      RefSCMatrix C(SpinCase2 S);

      RefSCMatrix V_genref_projector2(SpinCase2 pairspin);
      RefSCMatrix V_transformed_by_C(SpinCase2 pairspin);
      RefSymmSCMatrix X_transformed_by_C(SpinCase2 pairspin);
      RefSymmSCMatrix B_transformed_by_C(SpinCase2 pairspin);
      /// computes the projected contribution to the energy.
      double compute_DC_energy_GenRefansatz2();

      /// This function computes the "old" General_PT2R12 correction, i.e. the one invoking projector 1.
      double energy_PT2R12_projector1(SpinCase2 pairspin);
      double energy_PT2R12_projector2(SpinCase2 pairspin);

      /** methods for spin-free algorithm */
      double energy_PT2R12_projector2_spinfree();
      RefSCMatrix V_genref_projector2();
      /** computes t^pq_rs * Gamma^rs_vx * f^x_w * t^vw_tu; */
      RefSCMatrix X_term_Gamma_F_T();
      RefSymmSCMatrix X_transformed_by_C();
      RefSCMatrix sf_B_others();

      // TODO reimplement using native spin-free densities from Psi3
      RefSymmSCMatrix rdm1_gg_sf();  // return spin-free 1/2 rdm
      RefSymmSCMatrix rdm1_sf();
      RefSCMatrix rdm1_sf_2spaces(const Ref<OrbitalSpace> bspace, const Ref<OrbitalSpace> kspace);

      RefSymmSCMatrix rdm2_sf();
      // return 2-RDM in certain spaces; all the spaces should be subsets of 1-RDM/2-RDM orbital space
      RefSCMatrix rdm2_sf_4spaces(const Ref<OrbitalSpace> b1space, const Ref<OrbitalSpace> b2space, const Ref<OrbitalSpace> k1space, const Ref<OrbitalSpace> k2space);
      ///  return a * Gamma(pq; rs) + b Gamma(p;r) Gamma(q;s) + c Gamma(p; s) Gamma(q; r)
      RefSCMatrix rdm2_sf_4spaces_int(const double a, const double b, double const c,
                                                  const Ref<OrbitalSpace> b1space,
                                                  const Ref<OrbitalSpace> b2space,
                                                  const Ref<OrbitalSpace> k1space,
                                                  const Ref<OrbitalSpace> k2space);

      ///permute a 2rdm matrix specified by 4 orbital spaces
      template<Tensor4_Permute HowPermute>
      RefSCMatrix rdm2_sf_4spaces_int_permu(RefSCMatrix rdm2_4space_int,
                                            const Ref<OrbitalSpace> b1space,
                                            const Ref<OrbitalSpace> b2space,
                                            const Ref<OrbitalSpace> k1space,
                                            const Ref<OrbitalSpace> k2space);
      /// check the G1 * G2 temrs for RHF case; every element should sum up to zero.
      void check_sf_r12_rdm();


      /** compute CABS singles correction using Fock operator as H0 */
      double energy_cabs_singles(SpinCase1 spin);
      /// compute CABS singles correction using two-body operators in H0
      double energy_cabs_singles_twobody_H0();

      /// Returns Hcore in MO basis
      RefSymmSCMatrix hcore_mo();
      RefSymmSCMatrix hcore_mo(SpinCase1 spin);
      /// molecular integrals in chemist's notation
      RefSymmSCMatrix moints();  // closed shell case
      RefSCMatrix moints(SpinCase2 pairspin);
      /// This returns <space1 space2 || space1 space2>
      RefSCMatrix g(SpinCase2 pairspin,
                    const Ref<OrbitalSpace>& space1,
                    const Ref<OrbitalSpace>& space2);
      /// This returns <bra1 bra2 || ket1 ket2>
      RefSCMatrix g(SpinCase2 pairspin,
                    const Ref<OrbitalSpace>& bra1,
                    const Ref<OrbitalSpace>& bra2,
                    const Ref<OrbitalSpace>& ket1,
                    const Ref<OrbitalSpace>& ket2);
      /**
       * Fock matrix in the same space as given by rdm1()
       */
      RefSCMatrix f(SpinCase1 spin);

      /*
       * phi truncated in lambda: terms with three-particle lambda's or higher or terms with
       * squares (or higher) of two-particle lambda's are neglected.
       * this version does not uses the 2-body cumulant (2-lambda).
       *
       * phi is reported in the same space as given by rdm2()
       */
      RefSymmSCMatrix phi_cumulant(SpinCase2 pairspin);
      /// gg block of phi (@sa R12IntEval::gg_space() )
      RefSymmSCMatrix phi_gg(SpinCase2 spin);

      /// recomputes the energy from densities obtained from reference()
      double energy_recomputed_from_densities();

      /// computes the Brillouin condition matrix = <a_p^q H>
      void brillouin_matrix();

      /// computes the energy using the Hylleraas matrix
      /// @param hmat Hylleraas matrix
      /// @param pairspin SpinCase2
      /// @param print_pair_energies if true, will print pair energies. The default is true.
      double compute_energy(const RefSCMatrix &hmat,
                            SpinCase2 pairspin,
                            bool print_pair_energies = true,
                            std::ostream& os = ExEnv::out0());

  };

} // end of namespace sc

#endif // end of header guard


// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End:
