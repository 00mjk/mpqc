//
// sr_r12intermediates_VXB_diag.h
//
// Copyright (C) 2013 Edward Valeev
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

#ifndef _mpqc_src_lib_chemistry_qc_mbptr12_srr12intermediatesVXBdiag_h
#define _mpqc_src_lib_chemistry_qc_mbptr12_srr12intermediatesVXBdiag_h

#include <TiledArray/eigen.h>
#include <TiledArray/algebra/conjgrad.h>
#include <TiledArray/expressions.h>

namespace sc {
  inline double get_element(const TA::Array<double, 4 >& array, const std::vector<std::size_t>& ele_idx)
  {
    const std::vector<std::size_t> tile_idx = array.trange().element_to_tile(ele_idx);
    return (array.find(tile_idx).get()[ele_idx]);
  }

  inline double get_element(const TA::Array<double, 2 >& array, const std::vector<std::size_t>& ele_idx)
  {
    const std::vector<std::size_t> tile_idx = array.trange().element_to_tile(ele_idx);
    return (array.find(tile_idx).get()[ele_idx]);
  }

  template <typename T>
  typename SingleReference_R12Intermediates<T>::TArray2
  SingleReference_R12Intermediates<T>::Bpk_qk(const char* p, const char* q) {

    const double C_0 = 1.0 / 2.0;
    const double C_1 = 1.0 / 4.0;
    const double B_C1 = 0.5 * C_0 * C_0 + 1.5 * C_1 * C_1;
    const double B_C2 = 0.5 * C_0 * C_0 - 1.5 * C_1 * C_1;

    std::string rTr_pkql = std::string("<") + p + " k|rTr|" + q + " l>";
    std::string rTr_kpql = std::string("<k ") + p + "|rTr|" + q + " l>";

    std::string r2_phJk_ql = std::string("<") + p + "_hJ(p') k|r2|" + q + " l>";
    std::string r2_pkhJ_ql = std::string("<") + p + " k_hJ(p')|r2|" + q + " l>";
    //
    std::string r2_kphJ_ql = std::string("<k ") + p + "_hJ(p')|r2|" + q + " l>";
    std::string r2_khJp_ql = std::string("<k_hJ(p') ") + p + "|r2|" + q + " l>";
    //
    std::string r2_pk_qhJl = std::string("<") + p + " k|r2|" + q + "_hJ(p') l>";
    std::string r2_pk_qlhJ = std::string("<") + p + " k|r2|" + q + " l_hJ(p')>";
    //
    std::string r2_pk_lqhJ = std::string("<") + p + " k|r2|l " + q + "_hJ(p')>";
    std::string r2_pk_lhJq = std::string("<") + p + " k|r2|l_hJ(p') " + q + ">";

    std::string pk_PQ = std::string("<") + p + " k|r|p' q'>";
    std::string kp_PQ = std::string("<k ") + p + "|r|p' q'>";
    std::string  qk_PKQ = std::string("<") + q + " k|r|p'_K(r') q'>";
    std::string  kq_PKQ = std::string("<k ") + q + "|r|p'_K(r') q'>";

    std::string pk_Pn = std::string("<") + p + " k|r|p' n>";
    std::string kp_Pn = std::string("<k ") + p + "|r|p' n>";
    std::string  qk_PFn = std::string("<") + q + " k|r|p'_F(r') n>";
    std::string  kq_PFn = std::string("<k ") + q + "|r|p'_F(r') n>";

    std::string pk_mA = std::string("<") + p + " k|r|m a'>";
    std::string kp_mA = std::string("<k ") + p + "|r|m a'>";
    std::string  qk_mFA = std::string("<") + q + " k|r|m_F(n) a'>";
    std::string  kq_mFA = std::string("<k ") + q + "|r|m_F(n) a'>";

    std::string pk_pq = std::string("<") + p + " k|r|p b>";
    std::string kp_pq = std::string("<k ") + p + "|r|p b>";
    std::string  qk_pFb = std::string("<") + q + " k|r|p_F(r) b>";
    std::string  kq_pFb = std::string("<k ") + q + "|r|p_F(r) b>";

    std::string  qk_mFPA = std::string("<") + q + " k|r|m_F(p') a'>";
    std::string  kq_mFPA = std::string("<k ") + q + "|r|m_F(p') a'>";
    //
    std::string qk_mA = std::string("<") + q + " k|r|m a'>";
    std::string kq_mA = std::string("<k ") + q + "|r|m a'>";
    std::string  pk_mFPA = std::string("<") + p + " k|r|m_F(p') a'>";
    std::string  kp_mFPA = std::string("<k ") + p + "|r|m_F(p') a'>";

    std::string pk_Ab = std::string("<") + p + " k|r|a' b>";
    std::string kp_Ab = std::string("<k ") + p + "|r|a' b>";
    std::string  qk_AFb = std::string("<") + q + " k|r|a'_F(q) b>";
    std::string  kq_AFb = std::string("<k ") + q + "|r|a'_F(q) b>";
    //
    std::string qk_Ab = std::string("<") + q + " k|r|a' b>";
    std::string kq_Ab = std::string("<k ") + q + "|r|a' b>";
    std::string  pk_AFb = std::string("<") + p + " k|r|a'_F(q) b>";
    std::string  kp_AFb = std::string("<k ") + p + "|r|a'_F(q) b>";

    TArray2 B_pq =
          //          diag
          ( B_C1 * _4(rTr_pkql.c_str()) + B_C2 * _4(rTr_kpql.c_str())
          //           Q
          + 0.5 * (
              B_C1 * (_4(r2_phJk_ql.c_str()) + _4(r2_pkhJ_ql.c_str()))
            + B_C2 * (_4(r2_kphJ_ql.c_str()) + _4(r2_khJp_ql.c_str()))

            + B_C1 * (_4(r2_pk_qhJl.c_str()) + _4(r2_pk_qlhJ.c_str()))
            + B_C2 * (_4(r2_pk_lqhJ.c_str()) + _4(r2_pk_lhJq.c_str()))
                  )
           ) * _2("<k|I|l>")
          //           rKr_p'q'
          - ( B_C1 * (_4(pk_PQ.c_str()) * _4(qk_PKQ.c_str())
                    + _4(kp_PQ.c_str()) * _4(kq_PKQ.c_str()))
            + B_C2 * (_4(kp_PQ.c_str()) * _4(qk_PKQ.c_str())
                    + _4(pk_PQ.c_str()) * _4(kq_PKQ.c_str()))
            )
          //           rFr_p'n
          - ( B_C1 * (_4(pk_Pn.c_str()) * _4(qk_PFn.c_str())
                    + _4(kp_Pn.c_str()) * _4(kq_PFn.c_str()))
            + B_C2 * (_4(kp_Pn.c_str()) * _4(qk_PFn.c_str())
                    + _4(pk_Pn.c_str()) * _4(kq_PFn.c_str()))
             )
          //           rFr_mA
          + ( B_C1 * (_4(pk_mA.c_str()) * _4(qk_mFA.c_str())
                    + _4(kp_mA.c_str()) * _4(kq_mFA.c_str()))
            + B_C2 * (_4(kp_mA.c_str()) * _4(qk_mFA.c_str())
                    + _4(pk_mA.c_str()) * _4(kq_mFA.c_str()))
            )
          //           rFr_pb
          - ( B_C1 * (_4(pk_pq.c_str()) * _4(qk_pFb.c_str())
                    + _4(kp_pq.c_str()) * _4(kq_pFb.c_str()))
            + B_C2 * (_4(kp_pq.c_str()) * _4(qk_pFb.c_str())
                    + _4(pk_pq.c_str()) * _4(kq_pFb.c_str()))
            )
          //
          - ( B_C1 * (_4(pk_mA.c_str()) * _4(qk_mFPA.c_str())
                    + _4(kp_mA.c_str()) * _4(kq_mFPA.c_str()))
            + B_C2 * (_4(kp_mA.c_str()) * _4(qk_mFPA.c_str())
                    + _4(pk_mA.c_str()) * _4(kq_mFPA.c_str()))

            + B_C1 * (_4(qk_mA.c_str()) * _4(pk_mFPA.c_str())
                    + _4(kq_mA.c_str()) * _4(kp_mFPA.c_str()))
            + B_C2 * (_4(kq_mA.c_str()) * _4(pk_mFPA.c_str())
                    + _4(qk_mA.c_str()) * _4(kp_mFPA.c_str()))
             )
         //
         -  ( B_C1 * (_4(pk_Ab.c_str()) * _4(qk_AFb.c_str())
                    + _4(kp_Ab.c_str()) * _4(kq_AFb.c_str()))
            + B_C2 * (_4(kp_Ab.c_str()) * _4(qk_AFb.c_str())
                    + _4(pk_Ab.c_str()) * _4(kq_AFb.c_str()))

            + B_C1 * (_4(qk_Ab.c_str()) * _4(pk_AFb.c_str())
                    + _4(kq_Ab.c_str()) * _4(kp_AFb.c_str()))
            + B_C2 * (_4(kq_Ab.c_str()) * _4(pk_AFb.c_str())
                    + _4(qk_Ab.c_str()) * _4(kp_AFb.c_str()))
            )
          ;
    return B_pq;
  }

  template <typename T>
  typename SingleReference_R12Intermediates<T>::TArray4
  SingleReference_R12Intermediates<T>::Bpr_qs(const char* p, const char* q)
  {

    const double C_0 = 0.5;
    const double C_1 = 0.25;
    const double B_C1 = 0.5 * C_0 * C_0 + 1.5 * C_1 * C_1;
    const double B_C2 = 0.5 * C_0 * C_0 - 1.5 * C_1 * C_1;

    std::string rTr_pkql = std::string("<") + p + " k|rTr|" + q + " l>";
    std::string rTr_kpql = std::string("<k ") + p + "|rTr|" + q + " l>";

    std::string r2_phJk_ql = std::string("<") + p + "_hJ(p') k|r2|" + q + " l>";
    std::string r2_pkhJ_ql = std::string("<") + p + " k_hJ(p')|r2|" + q + " l>";
    //
    std::string r2_kphJ_ql = std::string("<k ") + p + "_hJ(p')|r2|" + q + " l>";
    std::string r2_khJp_ql = std::string("<k_hJ(p') ") + p + "|r2|" + q + " l>";
    //
    std::string r2_pk_qhJl = std::string("<") + p + " k|r2|" + q + "_hJ(p') l>";
    std::string r2_pk_qlhJ = std::string("<") + p + " k|r2|" + q + " l_hJ(p')>";
    //
    std::string r2_pk_lqhJ = std::string("<") + p + " k|r2|l " + q + "_hJ(p')>";
    std::string r2_pk_lhJq = std::string("<") + p + " k|r2|l_hJ(p') " + q + ">";

    std::string pk_PQ = std::string("<") + p + " k|r|p' q'>";
    std::string kp_PQ = std::string("<k ") + p + "|r|p' q'>";
    std::string  ql_PKQ = std::string("<") + q + " l|r|p'_K(r') q'>";
    std::string  lq_PKQ = std::string("<l ") + q + "|r|p'_K(r') q'>";

    std::string pk_Pn = std::string("<") + p + " k|r|p' n>";
    std::string kp_Pn = std::string("<k ") + p + "|r|p' n>";
    std::string  ql_PFn = std::string("<") + q + " l|r|p'_F(r') n>";
    std::string  lq_PFn = std::string("<l ") + q + "|r|p'_F(r') n>";

    std::string pk_mA = std::string("<") + p + " k|r|m a'>";
    std::string kp_mA = std::string("<k ") + p + "|r|m a'>";
    std::string  ql_mFA = std::string("<") + q + " l|r|m_F(n) a'>";
    std::string  lq_mFA = std::string("<l ") + q + "|r|m_F(n) a'>";

    std::string pk_pq = std::string("<") + p + " k|r|p b>";
    std::string kp_pq = std::string("<k ") + p + "|r|p b>";
    std::string  ql_pFb = std::string("<") + q + " l|r|p_F(r) b>";
    std::string  lq_pFb = std::string("<l ") + q + "|r|p_F(r) b>";

    std::string  ql_mFPA = std::string("<") + q + " l|r|m_F(p') a'>";
    std::string  lq_mFPA = std::string("<l ") + q + "|r|m_F(p') a'>";
    //
    std::string ql_mA = std::string("<") + q + " l|r|m a'>";
    std::string lq_mA = std::string("<l ") + q + "|r|m a'>";
    std::string  pk_mFPA = std::string("<") + p + " k|r|m_F(p') a'>";
    std::string  kp_mFPA = std::string("<k ") + p + "|r|m_F(p') a'>";

    std::string pk_Ab = std::string("<") + p + " k|r|a' b>";
    std::string kp_Ab = std::string("<k ") + p + "|r|a' b>";
    std::string  ql_AFb = std::string("<") + q + " l|r|a'_F(q) b>";
    std::string  lq_AFb = std::string("<l ") + q + "|r|a'_F(q) b>";
    //
    std::string ql_Ab = std::string("<") + q + " l|r|a' b>";
    std::string lq_Ab = std::string("<l ") + q + "|r|a' b>";
    std::string  pk_AFb = std::string("<") + p + " k|r|a'_F(q) b>";
    std::string  kp_AFb = std::string("<k ") + p + "|r|a'_F(q) b>";

    TArray4 Bpr_qs =
          //          diag
            B_C1 * _4(rTr_pkql.c_str()) + B_C2 * _4(rTr_kpql.c_str())
          //           Q
          + 0.5 * (
              B_C1 * (_4(r2_phJk_ql.c_str()) + _4(r2_pkhJ_ql.c_str()))
            + B_C2 * (_4(r2_kphJ_ql.c_str()) + _4(r2_khJp_ql.c_str()))

            + B_C1 * (_4(r2_pk_qhJl.c_str()) + _4(r2_pk_qlhJ.c_str()))
            + B_C2 * (_4(r2_pk_lqhJ.c_str()) + _4(r2_pk_lhJq.c_str()))
                  )
          //           rKr_p'q'
          - ( B_C1 * (_4(pk_PQ.c_str()) * _4(ql_PKQ.c_str())
                    + _4(kp_PQ.c_str()) * _4(lq_PKQ.c_str()))
            + B_C2 * (_4(kp_PQ.c_str()) * _4(ql_PKQ.c_str())
                    + _4(pk_PQ.c_str()) * _4(lq_PKQ.c_str()))
            )
          //           rFr_p'n
          - ( B_C1 * (_4(pk_Pn.c_str()) * _4(ql_PFn.c_str())
                    + _4(kp_Pn.c_str()) * _4(lq_PFn.c_str()))
            + B_C2 * (_4(kp_Pn.c_str()) * _4(ql_PFn.c_str())
                    + _4(pk_Pn.c_str()) * _4(lq_PFn.c_str()))
             )
          //           rFr_mA
          + ( B_C1 * (_4(pk_mA.c_str()) * _4(ql_mFA.c_str())
                    + _4(kp_mA.c_str()) * _4(lq_mFA.c_str()))
            + B_C2 * (_4(kp_mA.c_str()) * _4(ql_mFA.c_str())
                    + _4(pk_mA.c_str()) * _4(lq_mFA.c_str()))
            )
          //           rFr_pb
          - ( B_C1 * (_4(pk_pq.c_str()) * _4(ql_pFb.c_str())
                    + _4(kp_pq.c_str()) * _4(lq_pFb.c_str()))
            + B_C2 * (_4(kp_pq.c_str()) * _4(ql_pFb.c_str())
                    + _4(pk_pq.c_str()) * _4(lq_pFb.c_str()))
            )
          //
          - ( B_C1 * (_4(pk_mA.c_str()) * _4(ql_mFPA.c_str())
                    + _4(kp_mA.c_str()) * _4(lq_mFPA.c_str()))
            + B_C2 * (_4(kp_mA.c_str()) * _4(ql_mFPA.c_str())
                    + _4(pk_mA.c_str()) * _4(lq_mFPA.c_str()))

            + B_C1 * (_4(ql_mA.c_str()) * _4(pk_mFPA.c_str())
                    + _4(lq_mA.c_str()) * _4(kp_mFPA.c_str()))
            + B_C2 * (_4(lq_mA.c_str()) * _4(pk_mFPA.c_str())
                    + _4(ql_mA.c_str()) * _4(kp_mFPA.c_str()))
             )
         //
         -  ( B_C1 * (_4(pk_Ab.c_str()) * _4(ql_AFb.c_str())
                    + _4(kp_Ab.c_str()) * _4(lq_AFb.c_str()))
            + B_C2 * (_4(kp_Ab.c_str()) * _4(ql_AFb.c_str())
                    + _4(pk_Ab.c_str()) * _4(lq_AFb.c_str()))

            + B_C1 * (_4(ql_Ab.c_str()) * _4(pk_AFb.c_str())
                    + _4(lq_Ab.c_str()) * _4(kp_AFb.c_str()))
            + B_C2 * (_4(lq_Ab.c_str()) * _4(pk_AFb.c_str())
                    + _4(ql_Ab.c_str()) * _4(kp_AFb.c_str()))
            )
          ;
    return Bpr_qs;
  }

  template <typename T>
  std::pair<typename SingleReference_R12Intermediates<T>::TArray2,
            typename SingleReference_R12Intermediates<T>::TArray2>
  SingleReference_R12Intermediates<T>::V_diag() {

    TArray2 V_ij_ij_cabs = dotket(ij_xy("<i j|g|m a'>"), ij_xy("<i j|r|m a'>"));
    // don't need this in closed shell!
    //TArray2 V_ij_ij_cabs1 = dotket(ij_xy("<i j|g|a' m>"), ij_xy("<i j|r|a' m>"));

    TArray2 V_ij_ij = take(ij_xy("<i j|gr|p q>"), ij) - dotket(ij_xy("<i j|g|p q>"), ij_xy("<i j|r|p q>"))
                      - V_ij_ij_cabs("i,j") - V_ij_ij_cabs("j,i");

    TArray2 V_ij_ji_cabs = dotket(ij_xy("<i j|g|m a'>"), ij_xy("<i j|r|m a'>"), true);
    TArray2 V_ij_ji = take(ij_xy("<i j|gr|p q>"), ji) - dotket(ij_xy("<i j|g|p q>"), ij_xy("<i j|r|p q>"), true)
                      - V_ij_ji_cabs("i,j") - V_ij_ji_cabs("j,i");

    return std::make_pair(V_ij_ij,V_ij_ji);

  }

  template <typename T>
  std::pair<typename SingleReference_R12Intermediates<T>::TArray2,
            typename SingleReference_R12Intermediates<T>::TArray2>
  SingleReference_R12Intermediates<T>::X_diag() {

    TArray2 X_ij_ij_cabs = dotket(ij_xy("<i j|r|m a'>"), ij_xy("<i j|r|m a'>"));
    // don't need this in closed shell!
    //TArray2 X_ij_ij_cabs1 = dotket(ij_xy("<i j|r|a' m>"), ij_xy("<i j|r|a' m>"));

    TArray2 X_ij_ij = take(ij_xy("<i j|r2|p q>"), ij) - dotket(ij_xy("<i j|r|p q>"), ij_xy("<i j|r|p q>"))
                      - X_ij_ij_cabs("i,j") - X_ij_ij_cabs("j,i");

    TArray2 X_ij_ji_cabs = dotket(ij_xy("<i j|r|m a'>"), ij_xy("<i j|r|m a'>"), true);
    TArray2 X_ij_ji = take(ij_xy("<i j|r2|p q>"), ji) - dotket(ij_xy("<i j|r|p q>"), ij_xy("<i j|r|p q>"), true)
                      - X_ij_ji_cabs("i,j") - X_ij_ji_cabs("j,i");

    return std::make_pair(X_ij_ij,X_ij_ji);

  }

  template <typename T>
  std::pair<typename SingleReference_R12Intermediates<T>::TArray2,
            typename SingleReference_R12Intermediates<T>::TArray2>
  SingleReference_R12Intermediates<T>::B_diag() {

    /// this is incomplete at the moment
    TArray2 B_ij_ij = take(ij_xy("<i j|rTr|p q>"), ij) + take(ij_xy("<i_hJ(p') j|r2|p q>"), ij)
                      - dotket(ij_xy("<i j|r|p' q'>"), ij_xy("<i j|r|p' q'_K(r')>"));

    TArray2 B_ij_ji = take(ij_xy("<i j|rTr|p q>"), ji) + take(ij_xy("<i_hJ(p') j|r2|p q>"), ji);

    return std::make_pair(B_ij_ij,B_ij_ji);

  }

  namespace detail {
    /** this functor helps to implement conjugate gradient CABS singles solver
     */
    template<typename T>
    struct _CABS_singles_h0t1 {

        typedef TiledArray::Array<T, 2> Array;

        /**
         * @param h0_AB allvirt/allvirt Fock operator
         * @param h0_ij occ/occ Fock operator
         */
        _CABS_singles_h0t1(const Array& h0_AB, const Array& h0_ij) :
            H0_AB(h0_AB), H0_IJ(h0_ij) {
        }

        const Array& H0_AB;
        const Array& H0_IJ;

        /**
         * @param[in] T1 t_i^A
         * @param[out] R1 R_i^A
         */
        void operator()(const Array& T1, Array& R1) {
          R1 = T1("i,b") * H0_AB("b,a") - H0_IJ("i,j") * T1("j,a");
        }
    };

    /** this functor helps to implement orbital response
     */
    template<typename T>
    struct _OrbResponse {

        typedef TiledArray::Array<T, 2> Array2;
        typedef TiledArray::Array<T, 4> Array4;

        /**
         * @param f_AB virt/virt Fock operator
         * @param f_ij occ/occ Fock operator
         * @param g_ij_ab <ij|ab>
         * @param g_ia_jb <ia|jb>
         */
        _OrbResponse(const Array2& f_AB, const Array2& f_ij,
                     const Array4& g_ij_ab, const Array4& g_ia_jb) :
            F_AB(f_AB), F_IJ(f_ij), G_IJ_AB(g_ij_ab), G_IA_JB(g_ia_jb) {
        }

        const Array2& F_AB;
        const Array2& F_IJ;
        const Array4& G_IJ_AB;
        const Array4& G_IA_JB;

        /**
         * @param[in] kappa kappa_i^a
         * @param[out] residual residual_i^a
         */
        void operator()(const Array2& kappa, Array2& residual) {
          residual =  kappa("i,b") * F_AB("b,a") - F_IJ("i,j") * kappa("j,a")
          + 4.0 * G_IJ_AB("i,j,a,b") *  kappa("j,b")
                   -       G_IJ_AB("i,j,b,a") *  kappa("j,b")
                   -       G_IA_JB("i,a,j,b") *  kappa("j,b");
        }
    };

    /// makes a diagonal 2-index preconditioner: pc_x^y = -1/ ( <x|O1|x> - <y|O2|y> )
    template <typename T>
    struct diag_precond2 {
        typedef Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> EigenMatrixX;
        diag_precond2(const EigenMatrixX& O1_mat,
                      const EigenMatrixX& O2_mat) :
                          O1_mat_(O1_mat), O2_mat_(O2_mat) {
        }
        template <typename Index> T operator()(const Index& i) {
          return 1.0 / (- O1_mat_(i[0], i[0]) + O2_mat_(i[1], i[1]));
        }

      private:
        EigenMatrixX O1_mat_;
        EigenMatrixX O2_mat_;
    };

    /// makes a diagonal 4-index preconditioner: pc_xy^zw = -1/ ( <x|O1|x> + <y|O2|y> - <z|O3|z> - <w|O4|w> )
    template <typename T>
    struct diag_precond4 {
        typedef Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> EigenMatrixX;
        diag_precond4(const EigenMatrixX& O1_mat,
                      const EigenMatrixX& O2_mat,
                      const EigenMatrixX& O3_mat,
                      const EigenMatrixX& O4_mat) :
                          O1_mat_(O1_mat), O2_mat_(O2_mat),
                          O3_mat_(O3_mat), O4_mat_(O4_mat) {
        }
        template <typename Index> T operator()(const Index& i) {
          return 1.0 / (- O1_mat_(i[0], i[0]) - O2_mat_(i[1], i[1]) + O3_mat_(i[2], i[2]) + O4_mat_(i[3], i[3]));
        }

      private:
        EigenMatrixX O1_mat_;
        EigenMatrixX O2_mat_;
        EigenMatrixX O3_mat_;
        EigenMatrixX O4_mat_;
    };

    template<typename T>
    struct Orbital_relaxation_Abjai {

        typedef TiledArray::Array<T, 2> Array2;
        typedef TiledArray::Array<T, 4> Array4;

        /**
         * @param A_bjai
         */
        Orbital_relaxation_Abjai(const Array4& a_bjai) :
          A_bjai(a_bjai) {
        }

        const Array4& A_bjai;

        /**
         * @param[in] K_bj
         * @param[out] R1 R_i^a
         */
        void operator()(const Array2& K_bj, Array2& R1) {
          R1 = K_bj("b,j") * A_bjai("b,j,a,i");
        }
    };

    // e_ij = (e_i + e_j)
    template <typename T>
    struct e_ij {
        typedef Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> EigenMatrixX;
        e_ij(const EigenMatrixX& O1_mat, const EigenMatrixX& O2_mat):
             O1_mat_(O1_mat), O2_mat_(O2_mat) {}

        template <typename Index> T operator()(const Index& i) {
          return (O1_mat_(i[0], i[0]) + O2_mat_(i[1], i[1]));
        }

      private:
        EigenMatrixX O1_mat_;
        EigenMatrixX O2_mat_;
    };

  } // namespace sc::detail

  template <typename T>
  typename SingleReference_R12Intermediates<T>::TArray2
  SingleReference_R12Intermediates<T>::rdm1() {

    if (0) {
      {
      typedef TiledArray::Array<T,2> Array;
      Array FiA = _2("<i|F|A'>");
      Array FAi = _2("<A'|F|i>");
      std::cout << "<i|F|A'>:" << std::endl << FiA << std::endl;
      std::cout << "<A'|F|i>:" << std::endl << FAi << std::endl;
      Array FiA_2(FiA.get_world(), FiA.trange());
      FiA_2("i,A'") = FAi("A',i");
      std::cout << "<i|F|A'>=Perm(<A'|F|i>):" << std::endl << FiA_2 << std::endl;
      }
      {
      typedef TiledArray::Array<T,4> Array;
      Array g_ij_ab = _4("<i j|g|a b>");
      Array g_ab_ij = _4("<a b|g|i j>");
      std::cout << "<i j|g|a b>:" << std::endl << g_ij_ab << std::endl;
      std::cout << "<a b|g|i j>:" << std::endl << g_ab_ij << std::endl;
      Array g_ij_ab_2(g_ij_ab.get_world(), g_ij_ab.trange());
      g_ij_ab_2("i,j,a,b") = g_ab_ij("a,b,i,j");
      std::cout << "<i j|g|a b>=Perm(<a b|g|i j>):" << std::endl << g_ij_ab_2 << std::endl;
      Array should_be_zero = g_ij_ab("i,j,a,b") - g_ab_ij("a,b,i,j");
      std::cout << "<i j|g|a b> - Perm(<a b|g|i j>):" << std::endl << should_be_zero << std::endl;
      const double max_nonzero = norminf(should_be_zero("i,j,a,b"));
      std::cout << "|| <i j|g|a b> - Perm(<a b|g|i j>) ||_\infty = " << max_nonzero << std::endl;
      }
      {
      typedef TiledArray::Array<T,2> Array;
      Array mu_z_ij = _2("<i|mu_z|j>");
      Array gamma_ij = _2("<i|gamma|j>");
      const double mu_z_e = dot(mu_z_ij("i,j"), gamma_ij("i,j"));
      double mu_z_n = 0.0;
      Ref<Molecule> mol = r12world_->basis()->molecule();
      for(int a=0; a<mol->natom(); ++a) {
        mu_z_n += mol->Z(a) * mol->r(a, 2);
      }
      std::cout << "mu_z = " << -mu_z_e+mu_z_n << std::endl;
      }
    }

    TArray2 Fab = _2("<a|F|b>");
    TArray2 Iij = _2("<i|I|j>");
    //std::cout << "Fock(vir,vir)\n" << Fab << std::endl;
    //std::cout << "Idenity(occ,occ)\n" << Iij << std::endl;

    // can only ask for T1 with i in bra!
    // since we computed T1 CABS, they are expressed in terms of all virtuals = A'
    // if you turn off vir-CABS coupling, use a' (i.e. CABS only)
    TArray2 T1iA = _2("<i|T1|A'>");
    //t1_cabs_.print("T1(RefSCMatrix)");
    //std::cout << "T1(cabs)\n" << T1iA << std::endl;
    TArray2 T1ia = _2("<i|T1|a>");
    //std::cout << "T1(cabs) => i by a block\n" << T1ia << std::endl;

    // recompute E2(CABS) = T1_cabs . H1
    const double E2_cabs = 2.0 * dot(T1iA("i,A'"), _2("<i|F|A'>"));
    std::cout << "E2_cabs (recomputed) = " << E2_cabs << std::endl;

#if 0
    // recompute T1_cabs and re-recompute E2_cabs
    {
      typedef TiledArray::Array<T,2> Array;
      Array Fii = _2("<i|F|j>");
      Array FAA = _2("<A'|F|B'>");
      // this computes Z_i^A' = T_i^B' F_B'^A' - F_i^j T_j^A'
      detail::_CABS_singles_h0t1<double> cabs_singles_rhs_eval(FAA, Fii);

      TA::ConjugateGradientSolver<Array, detail::_CABS_singles_h0t1<double> > cg_solver;
      Array FiA = _2("<i|F|A'>");
      Array minus_FiA = -1.0 * FiA("i,A'");
      Array T1_recomp = FiA;

      // make preconditioner: Delta_iA = <i|F|i> - <A'|F|A'>
      typedef detail::diag_precond2<double> pceval_type; //!< evaluator of preconditioner
      typedef TA::Array<T, 2, LazyTensor<T, 2, pceval_type > > TArray2d;
      TArray2d Delta_iA(FiA.get_world(), FiA.trange());
      pceval_type Delta_iA_gen(TA::array_to_eigen(Fii),
                               TA::array_to_eigen(FAA));

      // construct local tiles
      for(auto t=Delta_iA.trange().tiles().begin();
          t!=Delta_iA.trange().tiles().end();
          ++t)
        if (Delta_iA.is_local(*t)) {
          std::array<std::size_t, 2> index; std::copy(t->begin(), t->end(), index.begin());
          madness::Future < typename TArray2d::value_type >
            tile((LazyTensor<T, 2, pceval_type >(&Delta_iA, index, &Delta_iA_gen)
                ));

          // Insert the tile into the array
          Delta_iA.set(*t, tile);
        }
      Array preconditioner = Delta_iA("i,A'");

#if 0
      std::cout << "FiA:\n" << FiA << std::endl;
      std::cout << "Fii:\n" << Fii << std::endl;
      std::cout << "FAA:\n" << FAA << std::endl;
      std::cout << "preconditioner:\n" << preconditioner << std::endl;
#endif

      // solves CABS singles equations T_i^B' F_B'^A' - F_i^j T_j^A' = -F_i^A' using CG
      auto resnorm = cg_solver(cabs_singles_rhs_eval,
                               minus_FiA,
                               T1_recomp,
                               preconditioner,
                               1e-10);
      std::cout << "Converged CG to " << resnorm << std::endl;
      const double E2_cabs = 2.0 * dot(T1_recomp("i,A'"), _2("<i|F|A'>")); // 2 accounts for spin
      std::cout << "E2_cabs (re-recomputed) = " << E2_cabs << std::endl;

      // compute the (2)_S dipole moment
      {
        const double mu_z_e = 2*dot(_2("<i|mu_z|A'>"), T1_recomp("i,A'"))
                            - dot(_2("<i|mu_z|j>"), T1_recomp("i,A'") * T1_recomp("j,A'") )
                            + dot(_2("<A'|mu_z|B'>"), T1_recomp("i,A'") * T1_recomp("i,B'") );
        std::cout << "Mu_z (2)_S = " << 2*mu_z_e << std::endl; // 2 accounts for spin degeneracy
        std::cout << "Mu_z ref = " << (dot(_2("<i|mu_z|j>"),_2("<i|gamma|j>")))
                  << std::endl; // 2 is included in gamma!
      }

      // compute orbital rotation multipliers in the (2)_S Lagrangian
      if (1) {
        TArray2 Tia = T1_recomp("j,A'") * _2("<A'|I|a>");
        TArray2 Xia_1 = 2* (_2("<i|F|j>") * Tia("j,a") - T1_recomp("i,B'") * _2("<B'|F|a>"))
                        + 2 * _2("<i|F|C'>") * T1_recomp("j,C'") * Tia("j,a")
                        - 2 * (2 * _4("<j i|g|B' a>") - _4("<j i|g|a B'>") + 2 * _4("<j a|g|B' i>") - _4("<j a|g|i B'>")) * T1_recomp("j,B'")
                        - 2 * (2 * _4("<B' i|g|C' a>") - _4("<B' i|g|a C'>")) * T1_recomp("j,B'") * T1_recomp("j,C'")
                        + 2 * (2 * _4("<j i|g|k a>") - _4("<j i|g|a k>")) * T1_recomp("j,B'") * T1_recomp("k,B'");
        std::cout << "Xia_1, should not be 0:\n" << Xia_1 << std::endl;

        TArray2 Faa = _2("<a|F|b>");
        TArray4 G_ij_ab = _4("<i j|g|a b>");
        TArray4 G_ia_jb = _4("<i a|g|j b>");
        detail::_OrbResponse<double> response_lhs_eval(Faa, Fii, G_ij_ab, G_ia_jb);

        TA::ConjugateGradientSolver<Array, detail::_OrbResponse<double> > cg_solver;
        Array kappa = Xia_1;

        // make preconditioner: Delta_ia = <i|F|i> - <a|F|a>
        typedef detail::diag_precond2<double> pceval_type; //!< evaluator of preconditioner
        typedef TA::Array<T, 2, LazyTensor<T, 2, pceval_type > > TArray2d;
        TArray2d Delta_ia(Xia_1.get_world(), Xia_1.trange());
        pceval_type Delta_ia_gen(TA::array_to_eigen(Fii),
                                 TA::array_to_eigen(Faa));

        // construct local tiles
        for(auto t=Delta_ia.trange().tiles().begin();
            t!=Delta_ia.trange().tiles().end();
            ++t)
          if (Delta_ia.is_local(*t)) {
            std::array<std::size_t, 2> index; std::copy(t->begin(), t->end(), index.begin());
            madness::Future < typename TArray2d::value_type >
              tile((LazyTensor<T, 2, pceval_type >(&Delta_ia, index, &Delta_ia_gen)
                  ));

            // Insert the tile into the array
            Delta_ia.set(*t, tile);
          }
        Array preconditioner = Delta_ia("i,a");

        // solves orbital response using CG
        auto resnorm = cg_solver(response_lhs_eval,
                                 Xia_1,
                                 kappa,
                                 preconditioner,
                                 1e-10);
        std::cout << "Converged CG to " << resnorm << std::endl;

        // verify the solution:
        {
          TArray2 res = Xia_1;
          response_lhs_eval(kappa, res);
          std::cout << "should be zero = " << TA::expressions::norm2(res("i,a") - Xia_1("i,a"));
        }

        const double mu_z_e = dot(kappa("i,a"), _2("<i|mu_z|a>"));
        std::cout << "mu_z_e (orb response) = " << 2*mu_z_e << std::endl;

      }
    }

    // now recompute T1_cabs and E2_cabs using only CABS for perturbation!
    {
      std::cout << "computing E2_cabs due to CABS only as the first-order space\n";
      typedef TiledArray::Array<T,2> Array;
      Array Fii = _2("<i|F|j>");
      Array FAA = _2("<a'|F|b'>");
      // this computes Z_i^a' = T_i^b' F_b'^a' - F_i^j T_j^a'
      detail::_CABS_singles_h0t1<double> cabs_singles_rhs_eval(FAA, Fii);

      TA::ConjugateGradientSolver<Array, detail::_CABS_singles_h0t1<double> > cg_solver;
      Array FiA = _2("<i|F|a'>");
      Array minus_FiA = -1.0 * FiA("i,a'");
      Array T1_recomp = FiA;

      // make preconditioner: Delta_iA = <i|F|i> - <a'|F|a'>
      typedef detail::diag_precond2<double> pceval_type; //!< evaluator of preconditioner
      typedef TA::Array<T, 2, LazyTensor<T, 2, pceval_type > > TArray2d;
      TArray2d Delta_iA(FiA.get_world(), FiA.trange());
      pceval_type Delta_iA_gen(TA::array_to_eigen(Fii),
                               TA::array_to_eigen(FAA));

      // construct local tiles
      for(auto t=Delta_iA.trange().tiles().begin();
          t!=Delta_iA.trange().tiles().end();
          ++t)
        if (Delta_iA.is_local(*t)) {
          std::array<std::size_t, 2> index; std::copy(t->begin(), t->end(), index.begin());
          madness::Future < typename TArray2d::value_type >
            tile((LazyTensor<T, 2, pceval_type >(&Delta_iA, index, &Delta_iA_gen)
                ));

          // Insert the tile into the array
          Delta_iA.set(*t, tile);
        }
      Array preconditioner = Delta_iA("i,a'");

#if 0
      std::cout << "FiA:\n" << FiA << std::endl;
      std::cout << "Fii:\n" << Fii << std::endl;
      std::cout << "FAA:\n" << FAA << std::endl;
      std::cout << "preconditioner:\n" << preconditioner << std::endl;
#endif

      // solves CABS singles equations T_i^B' F_B'^A' - F_i^j T_j^A' = -F_i^A' using CG
      auto resnorm = cg_solver(cabs_singles_rhs_eval,
                               minus_FiA,
                               T1_recomp,
                               preconditioner,
                               1e-10);
      std::cout << "Converged CG to " << resnorm << std::endl;
      const double E2_cabs = 2.0 * dot(T1_recomp("i,a'"), _2("<i|F|a'>")); // 2 accounts for spin
      std::cout << "E2_cabs (re-recomputed) = " << E2_cabs << std::endl;

      // compute the (2)_S dipole moment
      {
        const double mu_z_e = 2*dot(_2("<i|mu_z|a'>"), T1_recomp("i,a'"))
                            - dot(_2("<i|mu_z|j>"), T1_recomp("i,a'") * T1_recomp("j,a'") )
                            + dot(_2("<a'|mu_z|b'>"), T1_recomp("i,a'") * T1_recomp("i,b'") );
        std::cout << "Mu_z (2)_S = " << 2*mu_z_e << std::endl; // 2 accounts for spin degeneracy
      }

      // compute orbital rotation multipliers in the (2)_S Lagrangian
      if (1) {
        // only include the first-order terms in the Lagrangian derivative
        TArray2 Xia_1 = - 2 * T1_recomp("i,b'") * _2("<b'|F|a>")
                      - 2 * (2 * _4("<j i|g|b' a>") - _4("<j i|g|a b'>") + 2 * _4("<j a|g|b' i>") - _4("<j a|g|i b'>")) * T1_recomp("j,b'")
                      - 2 * (2 * _4("<b' i|g|c' a>") - _4("<b' i|g|a c'>")) * T1_recomp("j,b'") * T1_recomp("j,c'")
                      + 2 * (2 * _4("<j i|g|k a>") - _4("<j i|g|a k>")) * T1_recomp("j,b'") * T1_recomp("k,b'");
        std::cout << "Xia_1, should not be 0:\n" << Xia_1 << std::endl;

        TArray2 Faa = _2("<a|F|b>");
        TArray4 G_ij_ab = _4("<i j|g|a b>");
        TArray4 G_ia_jb = _4("<i a|g|j b>");
        detail::_OrbResponse<double> response_lhs_eval(Faa, Fii, G_ij_ab, G_ia_jb);

        TA::ConjugateGradientSolver<Array, detail::_OrbResponse<double> > cg_solver;
        Array kappa = Xia_1;

        // make preconditioner: Delta_ia = <i|F|i> - <a|F|a>
        typedef detail::diag_precond2<double> pceval_type; //!< evaluator of preconditioner
        typedef TA::Array<T, 2, LazyTensor<T, 2, pceval_type > > TArray2d;
        TArray2d Delta_ia(Xia_1.get_world(), Xia_1.trange());
        pceval_type Delta_ia_gen(TA::array_to_eigen(Fii),
                                 TA::array_to_eigen(Faa));

        // construct local tiles
        for(auto t=Delta_ia.trange().tiles().begin();
            t!=Delta_ia.trange().tiles().end();
            ++t)
          if (Delta_ia.is_local(*t)) {
            std::array<std::size_t, 2> index; std::copy(t->begin(), t->end(), index.begin());
            madness::Future < typename TArray2d::value_type >
              tile((LazyTensor<T, 2, pceval_type >(&Delta_ia, index, &Delta_ia_gen)
                  ));

            // Insert the tile into the array
            Delta_ia.set(*t, tile);
          }
        Array preconditioner = Delta_ia("i,a");

        // solves orbital response using CG
        auto resnorm = cg_solver(response_lhs_eval,
                                 Xia_1,
                                 kappa,
                                 preconditioner,
                                 1e-10);
        std::cout << "Converged CG to " << resnorm << std::endl;
        const double mu_z_e = dot(kappa("i,a"), _2("<i|mu_z|a>"));
        std::cout << "mu_z_e (orb reponse) = " << 2*mu_z_e << std::endl;

      }
    }

    // compute T1_obs and E2_obs
    {
      std::cout << "computing E2_obs (virtuals is the first-order space)\n";
      typedef TiledArray::Array<T,2> Array;
      Array Fii = _2("<i|F|j>");
      Array FAA = _2("<a|F|b>");
      // this computes Z_i^a = T_i^b F_b^a - F_i^j T_j^a
      detail::_CABS_singles_h0t1<double> cabs_singles_rhs_eval(FAA, Fii);

      TA::ConjugateGradientSolver<Array, detail::_CABS_singles_h0t1<double> > cg_solver;
      Array FiA = _2("<i|F|a>");
      Array minus_FiA = -1.0 * FiA("i,a");
      Array T1_obs = FiA;

      // make preconditioner: Delta_iA = <i|F|i> - <a|F|a>
      typedef detail::diag_precond2<double> pceval_type; //!< evaluator of preconditioner
      typedef TA::Array<T, 2, LazyTensor<T, 2, pceval_type > > TArray2d;
      TArray2d Delta_iA(FiA.get_world(), FiA.trange());
      pceval_type Delta_iA_gen(TA::array_to_eigen(Fii),
                               TA::array_to_eigen(FAA));

      // construct local tiles
      for(auto t=Delta_iA.trange().tiles().begin();
          t!=Delta_iA.trange().tiles().end();
          ++t)
        if (Delta_iA.is_local(*t)) {
          std::array<std::size_t, 2> index; std::copy(t->begin(), t->end(), index.begin());
          madness::Future < typename TArray2d::value_type >
            tile((LazyTensor<T, 2, pceval_type >(&Delta_iA, index, &Delta_iA_gen)
                ));

          // Insert the tile into the array
          Delta_iA.set(*t, tile);
        }
      Array preconditioner = Delta_iA("i,a");

#if 0
      std::cout << "FiA:\n" << FiA << std::endl;
      std::cout << "Fii:\n" << Fii << std::endl;
      std::cout << "FAA:\n" << FAA << std::endl;
      std::cout << "preconditioner:\n" << preconditioner << std::endl;
#endif

      // solves CABS singles equations T_i^B' F_B'^A' - F_i^j T_j^A' = -F_i^A' using CG
      auto resnorm = cg_solver(cabs_singles_rhs_eval,
                               minus_FiA,
                               T1_obs,
                               preconditioner,
                               1e-10);
      std::cout << "Converged CG to " << resnorm << std::endl;
      // same as the lagrangian since solved for T1 exactly
      const double E2_obs = 2.0 * dot(T1_obs("i,a"), _2("<i|F|a>")); // 2 accounts for spin
      std::cout << "E2_obs = " << E2_obs << std::endl;
      const double E0_obs = dot(_2("<i|F|j>"), _2("<i|gamma|j>")); // 2 included in gamma
      std::cout << "E0_obs = " << scprintf("%15.10lf",E0_obs) << std::endl;

      // compute the (2)_S dipole moment
      {
        const double mu_z_e = 2*dot(_2("<i|mu_z|a>"), T1_obs("i,a"))
                            - dot(_2("<i|mu_z|j>"), T1_obs("i,a") * T1_obs("j,a") )
                            + dot(_2("<a|mu_z|b>"), T1_obs("i,a") * T1_obs("i,b") );
        std::cout << scprintf("Mu_z (2)_S OBS = %25.15lf", 2*mu_z_e) << std::endl; // 2 accounts for spin degeneracy
        std::cout << scprintf("Mu_z ref = %25.15lf", (dot(_2("<i|mu_z|j>"),_2("<i|gamma|j>")))) << std::endl;
      }

      // compute orbital rotation multipliers in the (2)_S OBS Lagrangian
      if (1) {
        // only include the first-order terms in the Lagrangian derivative
        TArray2 Xia_1 = _2("<i|F|j>") * T1_obs("j,a") - T1_obs("i,b") * _2("<b|F|a>")
                        + 2 * _2("<i|F|c>") * T1_obs("j,c") * T1_obs("j,a")
                        + 2 * T1_obs("i,c") * T1_obs("j,c") * _2("<j|F|a>")
                        ;
        std::cout << "Xia_1, should not be 0:\n" << Xia_1 << std::endl;
        TArray2 Fia = _2("<i|F|a>");
        std::cout << "Fia, should be close to Xia_1:\n" << Fia << std::endl;

        TArray2 Faa = _2("<a|F|b>");
        TArray4 G_ij_ab = _4("<i j|g|a b>");
        TArray4 G_ia_jb = _4("<i a|g|j b>");
        detail::_OrbResponse<double> response_lhs_eval(Faa, Fii, G_ij_ab, G_ia_jb);

        TA::ConjugateGradientSolver<Array, detail::_OrbResponse<double> > cg_solver;
        Array kappa = Xia_1;

        // make preconditioner: Delta_ia = <i|F|i> - <a|F|a>
        typedef detail::diag_precond2<double> pceval_type; //!< evaluator of preconditioner
        typedef TA::Array<T, 2, LazyTensor<T, 2, pceval_type > > TArray2d;
        TArray2d Delta_ia(Xia_1.get_world(), Xia_1.trange());
        pceval_type Delta_ia_gen(TA::array_to_eigen(Fii),
                                 TA::array_to_eigen(Faa));

        // construct local tiles
        for(auto t=Delta_ia.trange().tiles().begin();
            t!=Delta_ia.trange().tiles().end();
            ++t)
          if (Delta_ia.is_local(*t)) {
            std::array<std::size_t, 2> index; std::copy(t->begin(), t->end(), index.begin());
            madness::Future < typename TArray2d::value_type >
              tile((LazyTensor<T, 2, pceval_type >(&Delta_ia, index, &Delta_ia_gen)
                  ));

            // Insert the tile into the array
            Delta_ia.set(*t, tile);
          }
        Array preconditioner = Delta_ia("i,a");

        // solves orbital response using CG
        auto resnorm = cg_solver(response_lhs_eval,
                                 Xia_1,
                                 kappa,
                                 preconditioner,
                                 1e-10);
        std::cout << "Converged CG to " << resnorm << std::endl;
        std::cout << scprintf("E_2 (orb reponse) = %25.15lf", 2*dot(kappa("i,a"), _2("<i|F|a>"))) << std::endl;
        const double mu_z_e = dot(kappa("i,a"), _2("<i|mu_z|a>"));
        std::cout << scprintf("mu_z_e (orb reponse) = %25.15lf", 2*mu_z_e) << std::endl;

      }
    }

    if (1) {
      for(int i=0; i<10; ++i) {
        TArray4 should_be_zero = _4("<b j|g|a i>") - _4("<b i|g|a j>");
        std::cout << "should be 0: " << TA::expressions::norminf(should_be_zero("b,j,a,i")) << std::endl;
      }
    }
#endif

    // this now works ... thanks Justus!
#if 1
    //TArray4 A = _2("<i|I|j>") * _2("<a|F|b>") - _4("<i j|g|a b>");
    //std::cout << "A\n" << A << std::endl;
#endif

    /// this is just an example of how to compute the density
    TArray2 r2_i_j = _4("<i j|r|p q>") * _4("<k_F(p) j|r|p q>");
    //std::cout << "<ij|r|pq> . <kj|r|pq>\n" << r2_i_j << std::endl;

    // this is another random contraction, useful for non-diagonal X intermediate
    //TArray4 x = _4("<i j|r|p q>") * _4("<k l|r|p q>");

    // compute orbital relaxation contribution to 1e density
    // i.e. solve Abjai Dbj = Xai
    // close shell formula
    {
    std::cout << std::endl << "*** start calculating A_bjai ***" << std::endl;
    //
    TArray4 A_bjai = - _4("<b j|g|a i>")
                     - _4("<i j|g|b a>")
                     +  4.0 * _4("<j i|g|b a>")
                     + _2("<b|F|a>") * _2("<i|I|j>")
                     - _2("<a|I|b>") * _2("<i|F|j>")
                     ;
    //std::cout << "A^bj_ai from PSI formula \n" << A_bjai << std::endl;

//    TArray4 g_ij_ab = _4("<i j|g|a b>");
//    TArray4 g_ab_ij = _4("<a b|g|i j>");
//    TArray4 should_be_zero = g_ij_ab("i,j,a,b") - g_ab_ij("a,b,i,j");
//    TArray4 should_be_zero = _4("<b j|g|a i>") - _4("<b i|g|a j>");
//    std::cout << "<i j|g|a b> - Perm(<a b|g|i j>):" << std::endl << should_be_zero << std::endl;
//    const double max_nonzero = TiledArray::expressions::norminf(should_be_zero("b,j,a,i"));
//    std::cout << "|| <b j|g|a i> - <b i|g|a j> || infty = " << max_nonzero << std::endl;

    std::cout << std::endl << "*** end of calculating A_bjai ***" << std::endl;

    // make preconditioner: Delta_ai = 1 / (<a|F|a> - <i|F|i>)
    TArray2 mFij = - _2("<i|F|j>");
    TArray2 mFab = - _2("<a|F|b>");
    TArray2 Fai = _2("<a|F|i>");
    typedef detail::diag_precond2<double> pceval_type; //!< evaluator of preconditioner
    pceval_type Delta_ai_gen(TA::array_to_eigen(mFab),
                             TA::array_to_eigen(mFij));

    typedef TA::Array<T, 2, LazyTensor<T, 2, pceval_type > > TArray2d;
    TArray2d Delta_ai(Fai.get_world(), Fai.trange());

    // construct local tiles
    for(auto t = Delta_ai.trange().tiles().begin();
        t != Delta_ai.trange().tiles().end(); ++t)
      if (Delta_ai.is_local(*t)) {
        std::array<std::size_t, 2> index;
        std::copy(t->begin(), t->end(), index.begin());
        madness::Future < typename TArray2d::value_type >
          tile((LazyTensor<T, 2, pceval_type >(&Delta_ai, index, &Delta_ai_gen)
              ));

        // Insert the tile into the array
        Delta_ai.set(*t, tile);
      }
    TArray2 preconditioner = Delta_ai("a,i");

    detail::Orbital_relaxation_Abjai<double> Orbital_relaxation_Abjai(A_bjai);
    TA::ConjugateGradientSolver<TiledArray::Array<T,2>,
                                detail::Orbital_relaxation_Abjai<double> > cg_solver2;

    TArray2 mu_z_ai = _2("<a|mu_z|m>");
//    double mu_z_n = 0.0;
//    Ref<Molecule> mol = r12world_->basis()->molecule();
//    for(int a=0; a<mol->natom(); ++a) {
//      mu_z_n += mol->Z(a) * mol->r(a, 2);
//    }
    // electron charge = -1

    std::cout << std::endl << "*** start calculating X_ai" << std::endl;
    // CABS Singles orbital relaxation
#if 0
    TArray2 TiA = _2("<i|T1|A'>");
    TArray2 Tia = TiA("i,A'") * _2("<A'|I|a>");
    TArray2 Xai_E2 = 2.0 * (
                       _2("<A'|F|a>") * TiA("i,A'")
                     - Tia("m,a") * _2("<i|F|m>")
                     - (TiA("m,A'")* Tia("m,a")) * _2("<i|F|A'>")
                     //
                     + (2.0 * _4("<a m|g|i A'>") - _4("<a m|g|A' i>")
                       + 2.0 * _4("<a A'|g|i m>") - _4("<a A'|g|m i>")
                       ) * TiA("m,A'")
                     //
                     + (2.0 * _4("<a A'|g|i B'>") - _4("<a A'|g|B' i>"))
                       * TiA("m,A'") * TiA("m,B'")
                     //
                     - (2.0 *_4("<a n|g|i m>") - _4("<a n|g|m i>"))
                       * TiA("m,A'") * TiA("n,A'")
                     )
                     ;
    //std::cout << "X^a_i\n" << X_ai << std::endl;
    std::cout << "*** end calculating X_ai" << std::endl;

    // solve k_bj A_bjai = X_ai
    TArray2 Dbj_E2(Xai_E2.get_world(), Xai_E2.trange());
    auto resnorm_E2 = cg_solver2(Orbital_relaxation_Abjai,
                               Xai_E2,
                               Dbj_E2,
                               preconditioner,
                               1e-10);
//    std::cout << "Dbj " << Dbj << std::endl;
    std::cout << "(E2) Converged CG2 to " << resnorm_E2 << std::endl;

    const double mu_z_E2 = dot(mu_z_ai("a,m"), Dbj_E2("a,m"));
    std::cout << "mu_z (E2) = " << -mu_z_E2 * 2.0<< std::endl;
#endif

    // MP2 orbital response
#if 0
    //  Delta_ijab = - 1 / (<a|F|a> - <i|F|i> + <b|F|b> - <j|F|j>)
    typedef detail::diag_precond4<double> pc4eval_type;
    typedef TA::Array<T, 4, LazyTensor<T, 4, pc4eval_type > > TArray4d;
    TArray4d Delta_aibj(A_bjai.get_world(), A_bjai.trange());

    TArray2 Fij = _2("<i|F|j>");
    TArray2 Fab = _2("<a|F|b>");
    pc4eval_type Delta_aibj_gen(TA::array_to_eigen(Fab), TA::array_to_eigen(mFij),
                                TA::array_to_eigen(mFab), TA::array_to_eigen(Fij));

    // construct local tiles
    for(auto t = Delta_aibj.trange().tiles().begin();
        t != Delta_aibj.trange().tiles().end(); ++t)
      if (Delta_aibj.is_local(*t)) {
        std::array<std::size_t, 4> index;
        std::copy(t->begin(), t->end(), index.begin());
        madness::Future < typename TArray4d::value_type >
          tile((LazyTensor<T, 4, pc4eval_type >(&Delta_aibj, index, &Delta_aibj_gen)
              ));

        // Insert the tile into the array
        Delta_aibj.set(*t, tile);
      }

    // MP2 T2 amplitues
    TArray4 T2_abij = TA::expressions::multiply(_4("<a b|g|i j>"), Delta_aibj("a,i,b,j"));
    TArray2 D_mp2_ij =  (2.0 * T2_abij("a,b,m,i") - T2_abij("a,b,i,m"))
                        * T2_abij("a,b,m,j");
    TArray2 D_mp2_ab =  (2.0 * T2_abij("a,c,i,j") - T2_abij("c,a,i,j"))
                        * T2_abij("b,c,i,j");

    TArray2 Xai_mp2 = 2.0 * (
                    (2.0 * _4("<a m|g|c d>") - _4("<m a|g|c d>"))
                    * T2_abij("c,d,i,m")
                  //
                  - T2_abij("a,c,m,n")
                    * (2.0 * _4("<i c|g|m n>") - _4("<c i|g|m n>"))
                  //
                  - (2.0 *_4("<a n|g|i m>") - _4("<a n|g|m i>"))
                      * D_mp2_ij("n,m")
                   //
                  + (2.0 * _4("<a c|g|i d>") - _4("<a c|g|d i>"))
                     * D_mp2_ab("c,d")
                  )
                  ;

    TArray2 Dbj_mp2(Xai_mp2.get_world(), Xai_mp2.trange());
    auto resnorm_mp2 = cg_solver2(Orbital_relaxation_Abjai,
                               Xai_mp2,
                               Dbj_mp2,
                               preconditioner,
                               1e-10);
    std::cout << "(MP2) Converged CG2 to " << resnorm_mp2 << std::endl;
    const double mu_z_mp2 = dot(mu_z_ai("a,m"), Dbj_mp2("a,m"));
    std::cout << "mu_z (MP2) = " << -mu_z_mp2 * 2.0<< std::endl;
#endif

    // F12 orbital relaxation
    {
    const double C_0 = 1.0 / 2.0;
    const double C_1 = 1.0 / 4.0;

    // V contribution to F12 Xai
#if 0
    const double V_C1 = (0.5 * C_0 + 1.5 * C_1);
    const double V_C2 = (0.5 * C_0 - 1.5 * C_1);
    TArray2 V_ai =  ( V_C1 * _4("<a a'|r|m n>") + V_C2 * _4("<a' a|r|m n>"))
                     * _4("<m n|g|i a'>")
                  + ( V_C1 * _4("<i a'|r|m n>") + V_C2 * _4("<a' i|r|m n>"))
                     * _4("<m n|g|a a'>")
                  //
                  - ( (V_C1 * _4("<a m|gr|i n>") + V_C2 * _4("<m a|gr|i n>"))
                      * _2("<n|I|m>")
                    - _4("<a m|g|p q>")
                       * ( V_C1 * _4("<p q|r|i m>") + V_C2 * _4("<p q|r|m i>") )
                    - _4("<a m|g|a' n>")
                       * ( V_C1 * _4("<a' n|r|i m>") + V_C2 * _4("<a' n|r|m i>"))
                    - _4("<a m|g|n a'>")
                       * ( V_C1 * _4("<n a'|r|i m>") + V_C2 * _4("<n a'|r|m i>"))
                    )
                    //
                  - ( (V_C1* _4("<i m|gr|a n>") + V_C2 * _4("<m i|gr|a n>"))
                      * _2("<n|I|m>")
                  - _4("<i m|g|p q>")
                    * ( V_C1 * _4("<p q|r|a m>") + V_C2 * _4("<p q|r|m a>") )
                  - _4("<i m|g|a' n>")
                     * ( V_C1 * _4("<a' n|r|a m>") + V_C2 * _4("<a' n|r|m a>"))
                  - _4("<i m|g|n a'>")
                     * ( V_C1 * _4("<n a'|r|a m>") + V_C2 * _4("<n a'|r|m a>"))
                    );
#endif

    // X contribution to F12 Xai
    const double RR_C1 = 0.5 * C_0 * C_0 + 1.5 * C_1 * C_1;
    const double RR_C2 = 0.5 * C_0 * C_0 - 1.5 * C_1 * C_1;

#if 0
    TArray4 r2_akjl = _4("<a k|r2|j l>");
    TArray4 r2_kajl = _4("<k a|r2|j l>");
    TArray2 F_ij = _2("<i|F|j>");

    TArray4 r_pqjk = _4("<p q|r|j k>");
    TArray4 r_apnjk = _4("<a' n|r|j k>");
    TArray4 r_napjk = _4("<n a'|r|j k>");

    // 1/2 F^k_k R^ak_alpha beta R_ik^alpha beta
    TArray2 X_ai1 =  //(RR_C1 * r2_akjl("a,k,j,l") + RR_C2 * r2_kajl("k,a,j,l"))
                     //* _2("<l|I|k>") * F_ij("j,i")
                    (RR_C1 * r2_akjl("a,k,i,l") + RR_C2 * r2_kajl("k,a,i,l"))
                     * F_ij("l,k")

                    - (RR_C1 * _4("<a k|r|p q>") + RR_C2 * _4("<k a|r|p q>"))
                      * ( //r_pqjk("p,q,j,k") * F_ij("j,i")
                        //+
                          r_pqjk("p,q,i,l") * F_ij("l,k"))

                    - (RR_C1 * _4("<a k|r|a' n>") + RR_C2 * _4("<k a|r|a' n>"))
                      * ( //r_apnjk("a',n,j,k") * F_ij("j,i")
                        //+
                        r_apnjk("a',n,i,l") * F_ij("l,k"))

                    - (RR_C1 * _4("<a k|r|n a'>") + RR_C2 * _4("<k a|r|n a'>"))
                      * ( //r_napjk("n,a',j,k") * F_ij("j,i")
                        //+
                        r_napjk("n,a',i,l") * F_ij("l,k"))
                    ;

//    TArray2 X_ai2 = (RR_C1 * _4("<a b'|r|m n>") + RR_C2 * _4("<b' a|r|m n>"))
//                  * ( _4("<m n|r|j b'>") * _2("<j|F|i>")
//                    + _4("<m n|r|i c'>") * _2("<c'|F|b'>"));

    // 1/2 (F^m_m + F^n_n) R^ab'_mn R_ib'^mn
    TArray2 X_ai2 = (RR_C1 * _4("<a b'|r|m n>") + RR_C2 * _4("<b' a|r|m n>"))
                  * ( _4("<k n|r|i b'>") * _2("<k|F|m>")
                    + _4("<m l|r|i b'>") * _2("<l|F|n>"));

    TArray2 X_ai = X_ai1("a,i")
                 - X_ai2("a,i")
                 ;
#endif

    // B contribution to F12 Xai
    const double B_C1 = RR_C1;
    const double B_C2 = RR_C2;

    // 1st parts of B_ai:
    // R^ak_A'B' F^A'_C' R^ik_C'B' (A': all virtual orbital index)
#if 1
    TArray4 r_akPQ = _4("<a k|r|p' q'>");
    TArray4 r_kaPQ = _4("<k a|r|p' q'>");
    TArray4 rik_PKQ = _4("<i k|r|p'_K(r') q'>");

    TArray4 r_akPn = _4("<a k|r|p' n>");
    TArray4 r_kaPn = _4("<k a|r|p' n>");
    TArray4 rik_PFn = _4("<i k|r|p'_F(r') n>");

    TArray4 r_akmA = _4("<a k|r|m a'>");
    TArray4 r_kamA = _4("<k a|r|m a'>");
    TArray4 rik_mFA = _4("<i k|r|m_F(n) a'>");

    TArray4 r_akpq = _4("<a k|r|p b>");
    TArray4 r_kapq = _4("<k a|r|p b>");
    TArray4 rik_pFb = _4("<i k|r|p_F(r) b>");

    TArray4 rik_nFA = _4("<i k|r|n_F(p') a'>");
    //
    TArray4 r_ikmA = _4("<i k|r|m a'>");
    TArray4 rak_nFA = _4("<a k|r|n_F(p') a'>");
    TArray4 rka_nFA = _4("<k a|r|n_F(p') a'>");

    TArray4 r_akAb  = _4("<a k|r|a' b>");
    TArray4 r_kaAb  = _4("<k a|r|a' b>");
    TArray4 rik_AFb = _4("<i k|r|a'_F(q) b>");
    //
    TArray4 r_ikAb  = _4("<i k|r|a' b>");
    TArray4 rak_AFb = _4("<a k|r|a'_F(q) b>");
    TArray4 rka_AFb = _4("<k a|r|a'_F(q) b>");

    TArray2 B_ai1 =
          //          diag
          (  (B_C1 * _4("<a k|rTr|i l>") + B_C2 * _4("<k a|rTr|i l>"))
          //           Q
          + 0.5 * (
              B_C1 * (_4("<a_hJ(p') l|r2|i k>") + _4("<a l_hJ(p')|r2|i k>")
                    + _4("<a l|r2|i_hJ(p') k>") + _4("<a l|r2|i k_hJ(p')>"))

            + B_C2 * (_4("<l a_hJ(p')|r2|i k>") + _4("<l_hJ(p') a|r2|i k>")
                    + _4("<l a|r2|i_hJ(p') k>") + _4("<l a|r2|i k_hJ(p')>"))
                  )
           ) * _2("<k|I|l>")
          //           rKr_p'q'
          - ( B_C1 * (r_akPQ("a,k,p',q'") * rik_PKQ("i,k,p',q'")
                    + r_kaPQ("k,a,p',q'") * rik_PKQ("k,i,p',q'"))

            + B_C2 * (r_kaPQ("k,a,p',q'") * rik_PKQ("i,k,p',q'")
                    + r_akPQ("a,k,p',q'") * rik_PKQ("k,i,p',q'"))
            )
          //           rFr_p'n
          - ( B_C1 * (r_akPn("a,k,p',n") * rik_PFn("i,k,p',n")
                    + r_kaPn("k,a,p',n") * rik_PFn("k,i,p',n"))

            + B_C2 * (r_kaPn("k,a,p',n") * rik_PFn("i,k,p',n")
                    + r_akPn("a,k,p',n") * rik_PFn("k,i,p',n"))
             )
          //           rFr_mA
          + ( B_C1 * (r_akmA("a,k,m,a'") * rik_mFA("i,k,m,a'")
                    + r_kamA("k,a,m,a'") * rik_mFA("k,i,m,a'"))

            + B_C2 * (r_kamA("k,a,m,a'") * rik_mFA("i,k,m,a'")
                    + r_akmA("a,k,m,a'") * rik_mFA("k,i,m,a'"))
            )
          //           rFr_pb
          - ( B_C1 * (r_akpq("a,k,p,b") * rik_pFb("i,k,p,b")
                    + r_kapq("k,a,p,b") * rik_pFb("k,i,p,b"))

            + B_C2 * (r_kapq("k,a,p,b") * rik_pFb("i,k,p,b")
                    + r_akpq("a,k,p,b") * rik_pFb("k,i,p,b"))
            )
          //
          -      ( B_C1 * (r_akmA("a,k,n,a'") * rik_nFA("i,k,n,a'")
                         + r_kamA("k,a,n,a'") * rik_nFA("k,i,n,a'"))

                 + B_C2 * (r_kamA("k,a,n,a'") * rik_nFA("i,k,n,a'")
                         + r_akmA("a,k,n,a'") * rik_nFA("k,i,n,a'"))

                 + B_C1 * (r_ikmA("i,k,n,a'") * rak_nFA("a,k,n,a'")
                         + r_ikmA("k,i,n,a'") * rka_nFA("k,a,n,a'"))

                 + B_C2 * (r_ikmA("k,i,n,a'") * rak_nFA("a,k,n,a'")
                         + r_ikmA("i,k,n,a'") * rka_nFA("k,a,n,a'"))
                  )
         //
         -      ( B_C1 * (r_akAb("a,k,a',b") * rik_AFb("i,k,a',b")
                        + r_kaAb("k,a,a',b") * rik_AFb("k,i,a',b"))

                + B_C2 * (r_kaAb("k,a,a',b") * rik_AFb("i,k,a',b")
                        + r_akAb("a,k,a',b") * rik_AFb("k,i,a',b"))
                        //
                + B_C1 * (r_ikAb("i,k,a',b") * rak_AFb("a,k,a',b")
                        + r_ikAb("k,i,a',b") * rka_AFb("k,a,a',b"))

                + B_C2 * (r_ikAb("k,i,a',b") * rak_AFb("a,k,a',b")
                        + r_ikAb("i,k,a',b") * rka_AFb("k,a,a',b"))
                 )
          ;

//    // test codes for computing B_ai for H2O molecule
//   const char* a = "a";
//   const char* i = "i";
//   TArray2 B_ia = Bpk_qk(i,a);
//   TArray2 B_ai = Bpk_qk(a,i);
//   TArray2 B_ai2 = B_ai("a,i") + B_ia("i,a");
//
//   TArray4 B_akil = Bpr_qs(a,i);
//   TArray2 B_ai2 = B_akil("a,k,i,l") * _2("<k|I|l");
//
//   const char* i = "i";
//   const char* j = "j";
//   TArray4 B_ijkl = Bpr_qs(i,j);
//
//   double sum_Bijij = 0;
//   std::cout << "B_ijkl" << std::endl;
//   for (std::size_t i = 0; i < 5; ++i) {
//     for (std::size_t j = 0; j < 5; ++j) {
//       std::vector<std::size_t> indices(4);
//       indices[0] = indices[2] = i;
//       indices[1] = indices[3] = j;
//       sum_Bijij += get_element(B_ijkl, indices);
//     }
//   }
//   std::cout << "Bijkl sum: " << sum_Bijij << std::endl;

#endif

    // 2nd parts of B_ai:

    // Formula 1
    // 1/2 R^ab'_ow F^c'_i R^c'b'_ow + 1/2 R^ab'_ow F^C'_b' R^iC'_ow
    // 1/2 R^ib'_ow F^C'_a R^C'b'_ow + 1/2 R^ib'_ow F^c'_b' R^ab'_ow
#if 1
    TArray2 B_ai2 = (B_C1 * _4("<a b'|r|m n>") + B_C2 * _4("<b' a|r|m n>"))
                    * _4("<m n|r|i_F(c') b'>")
                    //
                  + (B_C1 * _4("<a b'|r|m n>") + B_C2 * _4("<b' a|r|m n>"))
                    * _4("<m n|r|i b'_F(c')>")
                  + (B_C1 * _4("<a b'|r|m n>") + B_C2 * _4("<b' a|r|m n>"))
                    * _4("<m n|r|i b'_F(c)>")

                    // //
                  + (B_C1 * _4("<a_F(c') b'|r|m n>") + B_C2 *_4("<b' a_F(c')|r|m n>"))
                     * _4("<m n|r|i b'>")
                  + (B_C1 * _4("<a_F(c) b'|r|m n>") + B_C2 *_4("<b' a_F(c)|r|m n>"))
                     * _4("<m n|r|i b'>")
                    //
                  + (B_C1 * _4("<a b'_F(c')|r|m n>") + B_C2 * _4("<b'_F(c') a|r|m n>"))
                     * _4("<m n|r|i b'>")
                    ;
#endif

    // Formula 2: using Approximation C
#if 0
    const double RFR_C1 = C_0 * C_0 + 3.0 * C_1 * C_1;
    const double RFR_C2 = C_0 * C_0 - 3.0 * C_1 * C_1;

    TArray2 B_ai2_C =
                  // - R F^p'_r' R
                  - (RFR_C1 * _4("<a p'|r|m n>") + RFR_C2 * _4("<p' a|r|m n>"))
                    * _4("<m n|r|i p'_F(r')>")

                  //   R F^n_m R
                  + (RFR_C1 * _4("<a b'|r|m n>") + RFR_C2 * _4("<b' a|r|m n>"))
                    * _4("<m n|r|i_F(n) b'>")

                  // - R F^r_p R
                  - (RFR_C1 * _4("<a p|r|m n>") + RFR_C2 * _4("<p a|r|m n>"))
                    * _4("<m n|r|i p_F(r)>")

                  // - R F^p'_m R
                  - (RFR_C1 * _4("<a b'|r|m n>") + RFR_C2 * _4("<b' a|r|m n>"))
                    * _4("<m n|r|i_F(p') b'>")
                    //
                  - _4("<a_F(p') b'|r|m n>")
                    * (RFR_C1 * _4("<m n|r|i b'>") + RFR_C2 * _4("<m n|r|b' i>"))

                  // - R F^q_c' R
                  - (RFR_C1 * _4("<a b'|r|m n>") + RFR_C2 * _4("<b' a|r|m n>"))
                    * _4("<m n|r|i b'_F(q)>")
                    //
                  - _4("<a b'_F(q)|r|m n>")
                    * (RFR_C1 * _4("<m n|r|i b'>") + RFR_C2 * _4("<m n|r|b' i>"))
                    ;
#endif

    // sum of part 1 and 2 from B contribution to Xai
    // using formula 1
    TArray2 B_ai = B_ai1("a,i") * 2.0 - B_ai2("a,i");
    // using Approxmiation C
    //TArray2 B_ai = (B_ai1("a,i") + B_ai2_C("a,i")) * 2.0;

    // contribution from f12 density
#if 0
    TArray2 D_f12_ij =  (B_C1 * _4("<i m|r2|j n>") + B_C2 * _4("<m i|r2|j n>"))
                        * _2("<n|I|m>")
                      - (B_C1 * _4("<i m|r|p q>") + B_C2 * _4("<m i|r|p q>"))
                        * _4("<p q|r|j m>")
                      - (B_C1 * _4("<i m|r|a' n>") + B_C2 * _4("<m i|r|a' n>"))
                        * _4("<a' n|r|j m>")
                      - (B_C1 * _4("<i m|r|n a'>") + B_C2 * _4("<m i|r|n a'>"))
                        * _4("<n a'|r|j m>");

//    double sum_D_f12_ij = 0;
//    std::cout << "D_f12_ij" << std::endl;
//    for (std::size_t i = 0; i < 5; ++i) {
//      std::vector<std::size_t> indices(2);
//      indices[0] = indices[1] = i;
//      sum_D_f12_ij += get_element(D_f12_ij, indices);
//    }
//    std::cout << "D_f12_ij sum: " << sum_D_f12_ij << std::endl;
//
//    TArray2 D_f12_AB = (B_C1 * _4("<A' C'|r|m n>") + B_C2 * _4("<C' A'|r|m n>"))
//                        * _4("<m n|r|B' C'>")
//                      - (B_C1 * _4("<A' C'|r|m n>") + B_C2 * _4("<C' A'|r|m n>"))
//                        * _4("<m n|r|B' C'>");

    TArray2 D_f12_ab = (B_C1 * _4("<a c'|r|m n>") + B_C2 * _4("<c' a|r|m n>"))
                        * _4("<m n|r|b c'>");

    TArray2 D_f12_apbp = (B_C1 * _4("<a' c|r|m n>") + B_C2 * _4("<c a'|r|m n>"))
                         * _4("<m n|r|b' c>")
                       + (B_C1 * _4("<a' c'|r|m n>") + B_C2 * _4("<c' a'|r|m n>"))
                         * _4("<m n|r|b' c'>")
                        ;

    TArray2 D_f12_apb = (B_C1 * _4("<a' c'|r|m n>") + B_C2 * _4("<c' a'|r|m n>"))
                        * _4("<m n|r|b c'>");

    // test for f12 density
//    double sum_D_f12_ab = 0;
//    std::cout << "D_f12_ab" << std::endl;
//    for (std::size_t a = 0; a < 36; ++a) {
//      std::vector<std::size_t> indices(2);
//      indices[0] = indices[1] = a;
//      sum_D_f12_ab += get_element(D_f12_ab, indices);
//    }
//    std::cout << "D_f12_ab sum: " << sum_D_f12_ab << std::endl;
//
//    double sum_D_f12_apbp = 0;
//    std::cout << "D_f12_apbp" << std::endl;
//    for (std::size_t ap = 0; ap < 113; ++ap) {
//      std::vector<std::size_t> indices(2);
//      indices[0] = indices[1] = ap;
//      sum_D_f12_apbp += get_element(D_f12_apbp, indices);
//    }
//    std::cout << "D_f12_apbp sum: " << sum_D_f12_apbp << std::endl;

    TArray2 gdf12_ai = - (2.0 *_4("<a n|g|i m>") - _4("<a n|g|m i>"))
                         * D_f12_ij("n,m")
                       //
                       + (2.0 * _4("<a b|g|i c>") - _4("<a b|g|c i>"))
                         * D_f12_ab("b,c")
                       //
                       + (2.0 * _4("<a b'|g|i c'>") - _4("<a b'|g|c' i>"))
                         * D_f12_apbp("b',c'")
                       //
                       + (2.0 * _4("<a b'|g|i c>") - _4("<a b'|g|c i>"))
                         * D_f12_apb("b',c")
                       + (2.0 * _4("<a b|g|i c'>") - _4("<a b|g|c' i>"))
                         * D_f12_apb("c',b")
                       ;
#endif

    TArray2 Xai_f12 = // V_ai("a,i") * 2.0
                      // + X_ai("a,i") * 2.0
                      B_ai("a,i")
                      // + gdf12_ai("a,i") * 2.0

                     ;

    TArray2 Dbj_f12(Xai_f12.get_world(), Xai_f12.trange());
    auto resnorm_f12 = cg_solver2(Orbital_relaxation_Abjai,
                                  Xai_f12,
                                  Dbj_f12,
                                  preconditioner,
                                  1e-10);
    const double mu_z_f12 = dot(mu_z_ai("a,m"), Dbj_f12("a,m"));
    std::cout << std::endl << "***  "
              << "mu_z (F12) = " << -mu_z_f12 * 2.0
              << "  ***"<< std::endl;
    }

    }
    world_.gop.fence();

    return r2_i_j;
  }


  template <typename T>
  typename SingleReference_R12Intermediates<T>::TArray4
  SingleReference_R12Intermediates<T>::V_spinfree(bool symmetrize_p1_p2) {

    TArray4 V_ij_mn = _4("<i1 i2|gr|m1 m2>") - _4("<i1 i2|r|p1 p2>") * _4("<m1 m2|g|p1 p2>")
                    - _4("<i1 i2|r|m3_gamma(m) a'>") * _4("<m1 m2|g|m3 a'>");

    if (symmetrize_p1_p2)
      return 0.5 * (V_ij_mn("i1,i2,m1,m2") + V_ij_mn("i2,i1,m2,m1"));
    else
      return V_ij_mn;
  }

  template <typename T>
  typename SingleReference_R12Intermediates<T>::TArray4
  SingleReference_R12Intermediates<T>::X_spinfree(bool symmetrize_p1_p2) {

    TArray4 X_ij_kl = _4("<i1 i2|r2|j1 j2>") - _4("<i1 i2|r|p1 p2>") * _4("<j1 j2|r|p1 p2>")
                    - _4("<i1 i2|r|m3_gamma(m) a'>") * _4("<j1 j2|r|m3 a'>");

    if (symmetrize_p1_p2)
      return 0.5 * (X_ij_kl("i1,i2,j1,j2") + X_ij_kl("i2,i1,j2,j1"));
    else
      return X_ij_kl;
  }

  template <typename T>
  typename SingleReference_R12Intermediates<T>::TArray4
  SingleReference_R12Intermediates<T>::B_spinfree(bool symmetrize_p1_p2) {

    TArray4 B_ij_kl =

        // everything seems scaled up by factor of 2 relative to Eq.(12) in J. Chem. Phys. 135, 214105 (2011),
        // due to including particle 1 and particle 2 contributions?

        // diag                      Q
        _4("<i1 i2|rTr|j1 j2>") + 2.0 * _4("<i1 i2|r2|j1 j2_hJ(p')>")

        //           rKr
        -2.0 * _4("<i1 i2|r|r' s'>") * _4("<j1 j2|r|r' s'_K(p')>")

        //           rFr
        -2.0 * _4("<i1 i2|r|r s>") * _4("<j1 j2|r|r s_F(p)>")

        //           rFr_2, extra 2 due to bra-ket symmetrization
        -4.0 * _4("<i1 i2|r|r s>") * _4("<j1 j2|r|r s_F(a')>")

        //           rFGr
        - _4("<i1 i2|r|n_gamma(m) b'>") * _4("<j1 j2|r|n b'_F(a')>")

        //           rFGr_2
        - _4("<i1 i2|r|n_gamma(m) a'>") * _4("<j1 j2|r|n_F(p') a'>");

    B_ij_kl = 0.5 * (B_ij_kl("i1,i2,j1,j2") + B_ij_kl("i2,i1,j2,j1"));
    B_ij_kl = 0.5 * (B_ij_kl("i1,i2,j1,j2") + B_ij_kl("j1,j2,i1,i2"));

    if (symmetrize_p1_p2)
      return 0.5 * (B_ij_kl("i1,i2,j1,j2") + B_ij_kl("i2,i1,j2,j1"));
    else
      return B_ij_kl;
  }


}; // end of namespace sc

#endif // end of header guard


// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End:
