//
// Created by Chong Peng on 7/1/15.
//

#ifndef TILECLUSTERCHEM_CCSD_H
#define TILECLUSTERCHEM_CCSD_H

#include "../include/tiledarray.h"
#include "../common/namespaces.h"

#include "trange1_engine.h"
#include "mo_block.h"
#include "../ta_routines/tarray_block.h"

#include "ccsd_intermediates.h"
#include "diis_ccsd.h"
#include "utility.h"

namespace mpqc {
    namespace cc {

        // CCSD class that computed CCSD energy


        // Options
        // BlockSize = int, control the block size in MO, default 16
        // OccBlockSize = int, control the block size in Occ, overide BlockSize
        // VirBlockSize = int, control the block size in Vir, overide BlockSize
        // FrozenCore = bool, control if use frozen core, default False
        // Direct = bool , control if use direct approach, default True
        // DIIS_ndi = int, control the number of data sets to retain, default is 5
        // DIIS_dmp = float, see DIIS in TA
        // DIIS_strt = int, see DIIS in TA
        // DIIS_ngr = int, see DIIS in TA
        // DIIS_ngrdiis = int, see DIIS in TA
        // DIIS_mf = float, see DIIS in TA
        // LessMemory = bool, control if store large intermediate in straight approach, default is true
        // PrintDetail = bool, if do detail printing, default is false
        // Converge = double, convergence of CCSD energy, default is 1.0e-07

        template<typename Tile, typename Policy>
        class CCSD {

        public:

            using TArray = TA::DistArray<Tile,Policy>;

            typedef mpqc::TArrayBlock<Tile, Policy, mpqc::MOBlock> TArrayBlock;


            CCSD(const TArray &fock, const Eigen::VectorXd &ens,
                 const std::shared_ptr<TRange1Engine> &tre,
                 const std::shared_ptr<CCSDIntermediate<Tile, Policy>> &inter,
                 rapidjson::Document &options) :
                    fock_(fock), orbital_energy_(ens), trange1_engine_(tre), ccsd_intermediate_(inter), options_(std::move(options))
            {
//                auto mo_block = std::make_shared<mpqc::MOBlock>(*trange1_engine_);
//                fock_ = TArrayBlock(fock, mo_block);
            }


            TA::DIIS <mpqc::cc::T1T2<double, Tile, Policy>> get_diis(const madness::World & world){

                int n_diis, strt, ngr, ngrdiis;
                double dmp, mf;

                strt = options_.HasMember("DIIS_strt") ? options_["DIIS_strt"].GetInt() : 5;
                n_diis = options_.HasMember("DIIS_ndi") ? options_["DIIS_ndi"].GetInt() : 8;
                ngr = options_.HasMember("DIIS_ngr") ? options_["DIIS_ngr"].GetInt() : 2;
                ngrdiis = options_.HasMember("DIIS_ngrdiis") ? options_["DIIS_ngrdiis"].GetInt() : 1;
                dmp = options_.HasMember("DIIS_dmp") ? options_["DIIS_dmp"].GetDouble() : 0.0;
                mf = options_.HasMember("DIIS_mf") ? options_["DIIS_mf"].GetDouble() : 0.0;

                if(world.rank() == 0){
                    std::cout << "DIIS Starting Iteration:  " << strt << std::endl;
                    std::cout << "DIIS Storing Size:  " << n_diis << std::endl;
                    std::cout << "DIIS ngr:  " << ngr << std::endl;
                    std::cout << "DIIS ngrdiis:  " << ngrdiis << std::endl;
                    std::cout << "DIIS dmp:  " << dmp << std::endl;
                    std::cout << "DIIS mf:  " << mf << std::endl;
                }
                TA::DIIS <mpqc::cc::T1T2<double, Tile, Policy>> diis(strt,n_diis,0.0,ngr,ngrdiis);

                return diis;

            };

            // compute function
            virtual void compute(){

                TArray t1;
                TArray t2;

                auto direct = options_.HasMember("Direct") ? options_["Direct"].GetBool(): true;
                if(direct){
                    double ccsd_corr = compute_ccsd_direct(t1, t2);
                }
                else {
                    double ccsd_corr = compute_ccsd_nondirect(t1,t2);
                }

                ccsd_intermediate_->clean_two_electron();

            }


            // dummy way of doing CCSD
            // store all the integrals in memory
            // used as reference for development
            double compute_ccsd_straight(TArray &t1, TArray &t2) {

                auto n_occ = trange1_engine_->get_actual_occ();

                TArray g_abij = ccsd_intermediate_->get_abij();

                auto& world = g_abij.get_world();

                if(world.rank() == 0){
                    std::cout << "Use Straight CCSD Compute" <<std::endl;
                }

                TArray f_ai;
                f_ai("a,i") = fock_("a,i");

                world.gop.fence();

//      std::cout << g_abij << std::endl;

                TArray d1(f_ai.get_world(), f_ai.trange(), f_ai.get_shape(),
                           f_ai.get_pmap());
                // store d1 to local
                mpqc::cc::create_d_ai(d1, orbital_energy_, n_occ);

                TArray d2(world, g_abij.trange(),
                           g_abij.get_shape(), g_abij.get_pmap());
                // store d2 distributed
                mpqc::cc::create_d_abij(d2, orbital_energy_, n_occ);

                t1("a,i") = f_ai("a,i") * d1("a,i");
                t2("a,b,i,j") = g_abij("a,b,i,j") * d2("a,b,i,j");

//      std::cout << t1 << std::endl;
//      std::cout << t2 << std::endl;
                TArray tau;
                tau("a,b,i,j") = t2("a,b,i,j") + t1("a,i") * t1("b,j");

                double E0 = 0.0;
                double E1 = 2.0 * TA::dot(f_ai("a,i"), t1("a,i")) +
                            TA::dot(2.0 * g_abij("a,b,i,j") - g_abij("b,a,i,j"),
                                    tau("a,b,i,j"));
                double mp2 = E1;
                double dE = std::abs(E1 - E0);
//      std::cout << E1 << std::endl;

                // get all two electron integrals
                TArray g_ijkl = ccsd_intermediate_->get_ijkl();
                TArray g_abcd = ccsd_intermediate_->get_abcd();
                TArray g_iajb = ccsd_intermediate_->get_iajb();
                TArray g_iabc = ccsd_intermediate_->get_iabc();
                TArray g_aibc = ccsd_intermediate_->get_aibc();
                TArray g_ijak = ccsd_intermediate_->get_ijak();
                TArray g_ijka = ccsd_intermediate_->get_ijka();

//                 clean up three center integral after compute all two electron integrals
//                ccsd_intermediate_->clean_three_center();

                //optimize t1 and t2
                std::size_t iter = 0ul;
                double error = 1.0;
                TArray r1;
                TArray r2;

                auto diis = get_diis(world);

                bool less = options_.HasMember("LessMemory") ? options_["LessMemory"].GetBool() : true;

                if (world.rank() == 0) {
                    std::cout << "Start Iteration" << std::endl;
                    if(less){
                        std::cout << "Less Memory Approach: Yes" << std::endl;
                    }else{
                        std::cout << "Less Memory Approach: No" << std::endl;
                    }
                }
                while ((dE >= 1.0e-7 || error >= 1e-7)) {

                    //start timer
                    auto time0 = mpqc_time::now();

                    // intermediates for t1
                    // external index i and a
                    // vir index a b c d
                    // occ index i j k l
                    TArray h_ac, h_ki, h_kc;
                    {
                        h_ac("a,c") = -(2.0 * g_abij("c,d,k,l") - g_abij("c,d,l,k")) * tau("a,d,k,l");

                        h_ki("k,i") = (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * tau("c,d,i,l");

                        h_kc("k,c") = f_ai("c,k") + (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * t1("d,l");
                    }

                    // compute residual r1(n) = t1(n+1) - t1(n)
                    // external index i and a
                    {
                        r1("a,i") = -t1("a,i") + d1("a,i") * (
                                //
                                f_ai("a,i") - 2.0 * f_ai("c,k") * t1("c,i") * t1("a,k")
                                //
                                + h_ac("a,c") * t1("c,i") - t1("a,k") * h_ki("k,i")
                                //
                                + h_kc("k,c") * (2.0 * t2("c,a,k,i") - t2("c,a,i,k") + t1("c,i") * t1("a,k"))
                                //
                                + (2.0 * g_abij("c,a,k,i") - g_iajb("k,a,i,c")) * t1("c,k")
                                //
                                + (2.0 * g_iabc("k,a,c,d") - g_iabc("k,a,d,c")) * tau("c,d,k,i")
                                //
                                - (2.0 * g_ijak("k,l,c,i") - g_ijak("l,k,c,i")) * tau("c,a,k,l")
                        );
                    }

                    // intermediates for t2
                    // external index i j a b

                    TArray a_klij, b_abij, b_abcd, j_akic, k_kaic, T;
                    TArray g_ki, g_ac;

                    // avoid store b_abcd
                    if(less){

                        // compute intermediates
                        {

                            T("d,b,i,l") = 0.5 * t2("d,b,i,l") + t1("d,i") * t1("b,l");

                            a_klij("k,l,i,j") = g_ijkl("k,l,i,j")

                                                + g_ijka("k,l,i,c") * t1("c,j")

                                                + g_ijak("k,l,c,j") * t1("c,i")

                                                + g_abij("c,d,k,l") * tau("c,d,i,j");

                            b_abij("a,b,i,j") = g_abcd("a,b,c,d") * tau("c,d,i,j")

                                                - g_aibc("a,k,c,d") * tau("c,d,i,j") * t1("b,k")

                                                - g_iabc("k,b,c,d") * tau("c,d,i,j") * t1("a,k");

                            g_ki("k,i") = h_ki("k,i") + f_ai("c,k") * t1("c,i")

                                          + (2.0 * g_ijka("k,l,i,c") - g_ijka("l,k,i,c")) * t1("c,l");

                            g_ac("a,c") = h_ac("a,c") - f_ai("c,k") * t1("a,k")

                                          + (2.0 * g_aibc("a,k,c,d") - g_aibc("a,k,d,c")) * t1("d,k");

                            j_akic("a,k,i,c") = g_abij("a,c,i,k")

                                                - g_ijka("l,k,i,c") * t1("a,l")

                                                + g_aibc("a,k,d,c") * t1("d,i")

                                                - g_abij("c,d,k,l") * T("d,a,i,l")

                                                + 0.5 * (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * t2("a,d,i,l");

                            k_kaic("k,a,i,c") = g_iajb("k,a,i,c")

                                                - g_ijka("k,l,i,c") * t1("a,l")

                                                + g_iabc("k,a,d,c") * t1("d,i")

                                                - g_abij("d,c,k,l") * T("d,a,i,l");

                        }

                        // compute residual r2(n) = t2(n+1) - t2(n)
                        {

                            r2("a,b,i,j") = -t2("a,b,i,j") + d2("a,b,i,j") * (
                                    //
                                    g_abij("a,b,i,j")
                                    //
                                    + a_klij("k,l,i,j") * tau("a,b,k,l")
                                    //
                                    + b_abij("a,b,i,j")

                                    // permutation part
                                    //
                                    + (g_ac("a,c") * t2("c,b,i,j") - g_ki("k,i") * t2("a,b,k,j"))

                                    + (g_ac("b,c") * t2("c,a,j,i") - g_ki("k,j") * t2("b,a,k,i"))

                                    + (g_iabc("i,c,a,b") - g_iajb("k,b,i,c") * t1("a,k")) * t1("c,j")

                                    + (g_iabc("j,c,b,a") - g_iajb("k,a,j,c") * t1("b,k")) * t1("c,i")
                                    //
                                    - (g_ijak("i,j,a,k") + g_abij("a,c,i,k") * t1("c,j")) * t1("b,k")

                                    - (g_ijak("j,i,b,k") + g_abij("b,c,j,k") * t1("c,i")) * t1("a,k")

                                    + 0.5 * (2.0 * j_akic("a,k,i,c") - k_kaic("k,a,i,c")) *
                                      (2.0 * t2("c,b,k,j") - t2("b,c,k,j"))

                                    + 0.5 * (2.0 * j_akic("b,k,j,c") - k_kaic("k,b,j,c")) *
                                      (2.0 * t2("c,a,k,i") - t2("a,c,k,i"))

                                    - 0.5 * k_kaic("k,a,i,c") * t2("b,c,k,j") - k_kaic("k,b,i,c") * t2("a,c,k,j")

                                    - 0.5 * k_kaic("k,b,j,c") * t2("a,c,k,i") - k_kaic("k,a,j,c") * t2("b,c,k,i")
                            );
                        }
                    }
                    // store b_abcd to speed up calculation
                    else{

                        // compute intermediates
                        {

                            T("d,b,i,l") = 0.5 * t2("d,b,i,l") + t1("d,i") * t1("b,l");

                            a_klij("k,l,i,j") = g_ijkl("k,l,i,j")

                                                + g_ijka("k,l,i,c") * t1("c,j")

                                                + g_ijak("k,l,c,j") * t1("c,i")

                                                + g_abij("c,d,k,l") * tau("c,d,i,j");

                            b_abcd("a,b,c,d") = g_abcd("a,b,c,d")

                                                - g_aibc("a,k,c,d") * t1("b,k")

                                                - g_iabc("k,b,c,d") * t1("a,k");

                            g_ki("k,i") = h_ki("k,i") + f_ai("c,k") * t1("c,i")

                                          + (2.0 * g_ijka("k,l,i,c") - g_ijka("l,k,i,c")) * t1("c,l");

                            g_ac("a,c") = h_ac("a,c") - f_ai("c,k") * t1("a,k")

                                          + (2.0 * g_aibc("a,k,c,d") - g_aibc("a,k,d,c")) * t1("d,k");

                            j_akic("a,k,i,c") = g_abij("a,c,i,k")

                                                - g_ijka("l,k,i,c") * t1("a,l")

                                                + g_aibc("a,k,d,c") * t1("d,i")

                                                - g_abij("c,d,k,l") * T("d,a,i,l")

                                                + 0.5 * (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * t2("a,d,i,l");

                            k_kaic("k,a,i,c") = g_iajb("k,a,i,c")

                                                - g_ijka("k,l,i,c") * t1("a,l")

                                                + g_iabc("k,a,d,c") * t1("d,i")

                                                - g_abij("d,c,k,l") * T("d,a,i,l");

                        }

                        // compute residual r2(n) = t2(n+1) - t2(n)
                        {

                            r2("a,b,i,j") = -t2("a,b,i,j") + d2("a,b,i,j") * (
                                    //
                                    g_abij("a,b,i,j")
                                    //
                                    + a_klij("k,l,i,j") * tau("a,b,k,l")
                                    //
                                    + b_abcd("a,b,c,d") * tau("c,d,i,j")

                                    // permutation part
                                    //
                                    + (g_ac("a,c") * t2("c,b,i,j") - g_ki("k,i") * t2("a,b,k,j"))

                                    + (g_ac("b,c") * t2("c,a,j,i") - g_ki("k,j") * t2("b,a,k,i"))

                                    + (g_iabc("i,c,a,b") - g_iajb("k,b,i,c") * t1("a,k")) * t1("c,j")

                                    + (g_iabc("j,c,b,a") - g_iajb("k,a,j,c") * t1("b,k")) * t1("c,i")
                                    //
                                    - (g_ijak("i,j,a,k") + g_abij("a,c,i,k") * t1("c,j")) * t1("b,k")

                                    - (g_ijak("j,i,b,k") + g_abij("b,c,j,k") * t1("c,i")) * t1("a,k")

                                    + 0.5 * (2.0 * j_akic("a,k,i,c") - k_kaic("k,a,i,c")) *
                                      (2.0 * t2("c,b,k,j") - t2("b,c,k,j"))

                                    + 0.5 * (2.0 * j_akic("b,k,j,c") - k_kaic("k,b,j,c")) *
                                      (2.0 * t2("c,a,k,i") - t2("a,c,k,i"))

                                    - 0.5 * k_kaic("k,a,i,c") * t2("b,c,k,j") - k_kaic("k,b,i,c") * t2("a,c,k,j")

                                    - 0.5 * k_kaic("k,b,j,c") * t2("a,c,k,i") - k_kaic("k,a,j,c") * t2("b,c,k,i")
                            );
                        }
                    }

//        g_abij.get_world().gop.fence();

                    t1("a,i") = t1("a,i") + r1("a,i");
                    t2("a,b,i,j") = t2("a,b,i,j") + r2("a,b,i,j");
                    t1.truncate();
                    t2.truncate();

                    mpqc::cc::T1T2<double, Tile, Policy> t(t1, t2);
                    mpqc::cc::T1T2<double, Tile, Policy> r(r1, r2);
                    error = r.norm() / size(t);
                    diis.extrapolate(t, r);

                    //update t1 and t2
                    t1("a,i") = t.first("a,i");
                    t2("a,b,i,j") = t.second("a,b,i,j");

                    tau("a,b,i,j") = t2("a,b,i,j") + t1("a,i") * t1("b,j");

                    // recompute energy
                    E0 = E1;
                    E1 = 2.0 * TA::dot(f_ai("a,i"), t1("a,i"))
                         +
                         TA::dot((2.0 * g_abij("a,b,i,j") - g_abij("b,a,i,j")),
                                 tau("a,b,i,j"));
                    dE = std::abs(E0 - E1);
                    iter += 1ul;

                    auto time1 = mpqc_time::now();
                    auto duration = mpqc_time::duration_in_s(time0, time1);

                    if (world.rank() == 0) {
                        std::cout << iter << "  " << dE << "  " << error <<
                        "  " << E1 << "  " << duration << std::endl;
                    }

                    world.gop.fence();
//        std::cout << indent << scprintf("%-5.0f", iter) << scprintf("%-20.10f", Delta_E)
//        << scprintf("%-15.10f", E_1) << std::endl;

                }
                if (world.rank() == 0) {
                    std::cout << "MP2 Energy   " << mp2 << std::endl;
                    std::cout << "CCSD Energy  " << E1 << std::endl;
                }
                return E1;
            }


            // dummy way of doing CCSD
            // store all the integrals in memory
            // used as reference for development
            double compute_ccsd_nondirect(TArray &t1, TArray &t2) {

                bool print_detail = options_.HasMember("PrintDetail") ? options_["PrintDetail"].GetBool() : false;

                auto n_occ = trange1_engine_->get_actual_occ();

                TArray g_abij = ccsd_intermediate_->get_abij();

                auto& world = g_abij.get_world();

                if(world.rank() == 0){
                    std::cout << "Use Straight CCSD Compute" <<std::endl;
                }

                TArray f_ai;
                f_ai("a,i") = fock_("a,i");

                world.gop.fence();

//      std::cout << g_abij << std::endl;

                TArray d1(f_ai.get_world(), f_ai.trange(), f_ai.get_shape(),
                           f_ai.get_pmap());
                // store d1 to local
                mpqc::cc::create_d_ai(d1, orbital_energy_, n_occ);

                t1("a,i") = f_ai("a,i") * d1("a,i");

                t2= d_abij(g_abij,orbital_energy_,n_occ);

//      std::cout << t1 << std::endl;
//      std::cout << t2 << std::endl;
                TArray tau;
                tau("a,b,i,j") = t2("a,b,i,j") + t1("a,i") * t1("b,j");

                double E0 = 0.0;
                double E1 = 2.0 * TA::dot(f_ai("a,i"), t1("a,i")) +
                            TA::dot(2.0 * g_abij("a,b,i,j") - g_abij("b,a,i,j"),
                                    tau("a,b,i,j"));
                double mp2 = E1;
                double dE = std::abs(E1 - E0);
//      std::cout << E1 << std::endl;

                // get all two electron integrals
                TArray g_ijkl = ccsd_intermediate_->get_ijkl();
                TArray g_abcd = ccsd_intermediate_->get_abcd();
                TArray g_iajb = ccsd_intermediate_->get_iajb();
                TArray g_iabc = ccsd_intermediate_->get_iabc();
                TArray g_aibc = ccsd_intermediate_->get_aibc();
                TArray g_ijak = ccsd_intermediate_->get_ijak();
                TArray g_ijka = ccsd_intermediate_->get_ijka();

//                 clean up three center integral after compute all two electron integrals
//                ccsd_intermediate_->clean_three_center();

                //optimize t1 and t2
                std::size_t iter = 0ul;
                double error = 1.0;
                TArray r1;
                TArray r2;


                bool less = options_.HasMember("LessMemory") ? options_["LessMemory"].GetBool() : true;

                double converge = options_.HasMember("Converge") ? options_["Converge"].GetDouble() : 1.0e-7;

                if (world.rank() == 0) {
                    std::cout << "Start Iteration" << std::endl;
                    std::cout << "Convergence " << converge << std::endl;
                    if(less){
                        std::cout << "Less Memory Approach: Yes" << std::endl;
                    }else{
                        std::cout << "Less Memory Approach: No" << std::endl;
                    }
                }

                auto diis = get_diis(world);

                while (true) {

                    //start timer
                    auto time0 = mpqc_time::now();

                    TArray::wait_for_lazy_cleanup(world);
                    TArray::wait_for_lazy_cleanup(world);


                    TArray h_ki, h_ac;
                    {
                        // intermediates for t1
                        // external index i and a
                        // vir index a b c d
                        // occ index i j k l
                        TArray h_kc;


                        // compute residual r1(n) = t1(n+1) - t1(n)
                        // external index i and a
                        r1("a,i") = f_ai("a,i") - 2.0 * f_ai("c,k") * t1("c,i") * t1("a,k");

                        {
                            h_ac("a,c") = -(2.0 * g_abij("c,d,k,l") - g_abij("c,d,l,k")) * tau("a,d,k,l");
                            r1("a,i") += h_ac("a,c") * t1("c,i");
                        }

                        {
                            h_ki("k,i") = (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * tau("c,d,i,l");
                            r1("a,i") -= t1("a,k") * h_ki("k,i");
                        }
                        {

                            h_kc("k,c") = f_ai("c,k") + (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * t1("d,l");
                            r1("a,i") +=  h_kc("k,c") * (2.0 * t2("c,a,k,i") - t2("c,a,i,k") + t1("c,i") * t1("a,k"));
                        }


                        r1("a,i") += (2.0 * g_abij("c,a,k,i") - g_iajb("k,a,i,c")) * t1("c,k");

                        r1("a,i") += (2.0 * g_iabc("k,a,c,d") - g_iabc("k,a,d,c")) * tau("c,d,k,i");

                        r1("a,i") -= (2.0 * g_ijak("k,l,c,i") - g_ijak("l,k,c,i")) * tau("c,a,k,l");

                        r1("a,i") *= d1("a,i");

                        r1("a,i") -= t1("a,i");
                    }



                    // intermediates for t2
                    // external index i j a b


                    // compute residual r2(n) = t2(n+1) - t2(n)

                    //permutation part

                    {
                        r2("a,b,i,j") = (g_iabc("i,c,a,b") - g_iajb("k,b,i,c") * t1("a,k")) * t1("c,j");

                        r2("a,b,i,j") -= (g_ijak("i,j,a,k") + g_abij("a,c,i,k") * t1("c,j")) * t1("b,k");
                    }

                    {
                        // compute g intermediate
                        TArray g_ki, g_ac;

                        g_ki("k,i") = h_ki("k,i") + f_ai("c,k") * t1("c,i") + (2.0 * g_ijka("k,l,i,c") - g_ijka("l,k,i,c")) * t1("c,l");

                        g_ac("a,c") = h_ac("a,c") - f_ai("c,k") * t1("a,k") + (2.0 * g_aibc("a,k,c,d") - g_aibc("a,k,d,c")) * t1("d,k");

                        r2("a,b,i,j") += g_ac("a,c") * t2("c,b,i,j") - g_ki("k,i") * t2("a,b,k,j");

                    }



                    {
                        TArray j_akic;
                        TArray k_kaic;
                        // compute j and k intermediate
                        {
                            TArray T;

                            T("d,b,i,l") = 0.5 * t2("d,b,i,l") + t1("d,i") * t1("b,l");

                            j_akic("a,k,i,c") = g_abij("a,c,i,k");

                            j_akic("a,k,i,c") -= g_ijka("l,k,i,c") * t1("a,l");

                            j_akic("a,k,i,c") += g_aibc("a,k,d,c") * t1("d,i");

                            j_akic("a,k,i,c") -= g_abij("c,d,k,l") * T("d,a,i,l");

                            j_akic("a,k,i,c") += 0.5 * (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * t2("a,d,i,l");

                            k_kaic("k,a,i,c") = g_iajb("k,a,i,c")

                                                - g_ijka("k,l,i,c") * t1("a,l")

                                                + g_iabc("k,a,d,c") * t1("d,i")

                                                - g_abij("d,c,k,l") * T("d,a,i,l");
                            if(print_detail){
                                utility::print_size_info(T,"T");
                                utility::print_size_info(j_akic,"J_akic");
                                utility::print_size_info(k_kaic,"K_kaic");
                            }
                        }


                        r2("a,b,i,j") += 0.5 * (2.0 * j_akic("a,k,i,c") - k_kaic("k,a,i,c")) * (2.0 * t2("c,b,k,j") - t2("b,c,k,j"));

                        r2("a,b,i,j") += - 0.5 * k_kaic("k,a,i,c") * t2("b,c,k,j") - k_kaic("k,b,i,c") * t2("a,c,k,j");

                    }

                    // perform the permutation
                    r2("a,b,i,j") = r2("a,b,i,j") + r2("b,a,j,i");

                    r2("a,b,i,j") += g_abij("a,b,i,j");


                    {
                        TArray a_klij;
                        // compute a intermediate
                        a_klij("k,l,i,j") = g_ijkl("k,l,i,j");

                        a_klij("k,l,i,j") += g_ijka("k,l,i,c") * t1("c,j");

                        a_klij("k,l,i,j") += g_ijak("k,l,c,j") * t1("c,i");

                        a_klij("k,l,i,j") += g_abij("c,d,k,l") * tau("c,d,i,j");

                        r2("a,b,i,j") += a_klij("k,l,i,j") * tau("a,b,k,l");
                    }

                    {
                        // compute b intermediate
                        if (less) {
                            // avoid store b_abcd
                            TArray b_abij;
                            b_abij("a,b,i,j") = g_abcd("a,b,c,d") * tau("c,d,i,j");

                            b_abij("a,b,i,j") -= g_aibc("a,k,c,d") * tau("c,d,i,j") * t1("b,k");

                            b_abij("a,b,i,j") -= g_iabc("k,b,c,d") * tau("c,d,i,j") * t1("a,k");

                            if(print_detail){
                                utility::print_size_info(b_abij,"B_abij");
                            }

                            r2("a,b,i,j") += b_abij("a,b,i,j");
                        } else {

                            TArray b_abcd;

                            b_abcd("a,b,c,d") = g_abcd("a,b,c,d") - g_aibc("a,k,c,d") * t1("b,k") - g_iabc("k,b,c,d") * t1("a,k");

                            if(print_detail){
                                utility::print_size_info(b_abcd,"B_abcd");
                            }

                            r2("a,b,i,j") += b_abcd("a,b,c,d") * tau("c,d,i,j");
                        }

                    }

                    d_abij_inplace(r2, orbital_energy_, n_occ);

                    r2("a,b,i,j") -= t2("a,b,i,j");


                    t1("a,i") = t1("a,i") + r1("a,i");
                    t2("a,b,i,j") = t2("a,b,i,j") + r2("a,b,i,j");
                    t1.truncate();
                    t2.truncate();

                    // recompute energy
                    E0 = E1;
                    E1 = 2.0 * TA::dot(f_ai("a,i"), t1("a,i"))
                         +
                         TA::dot((2.0 * g_abij("a,b,i,j") - g_abij("b,a,i,j")),
                                 tau("a,b,i,j"));
                    dE = std::abs(E0 - E1);

                    if (iter == 0 && world.rank() == 0) {
                        std::cout << "iter " << "    deltaE    " << "            residual       "
                        << "      energy     " << " total/second "<<std::endl;
                    }


                    if(dE >= converge || error >= converge){

                        mpqc::cc::T1T2<double, Tile, Policy> t(t1, t2);
                        mpqc::cc::T1T2<double, Tile, Policy> r(r1, r2);
                        error = r.norm() / size(t);
                        diis.extrapolate(t, r);

                        //update t1 and t2
                        t1("a,i") = t.first("a,i");
                        t2("a,b,i,j") = t.second("a,b,i,j");

                        if(print_detail){
                            utility::print_size_info(r2,"R2");
                            utility::print_size_info(t2,"T2");
                        }

                        tau("a,b,i,j") = t2("a,b,i,j") + t1("a,i") * t1("b,j");

                        auto time1 = mpqc_time::now();
                        auto duration = mpqc_time::duration_in_s(time0, time1);

                        if (world.rank() == 0) {
                            std::cout << iter << "  " << dE << "  " << error <<
                            "  " << E1 << "  " << duration << std::endl;
                        }

                        iter += 1ul;
                    }
                    else{

                        auto time1 = mpqc_time::now();
                        auto duration = mpqc_time::duration_in_s(time0, time1);

                        if (world.rank() == 0) {
                            std::cout << iter << "  " << dE << "  " << error <<
                            "  " << E1 << "  " << duration << std::endl;
                        }


                        break;
                    }
                    world.gop.fence();
//        std::cout << indent << scprintf("%-5.0f", iter) << scprintf("%-20.10f", Delta_E)
//        << scprintf("%-15.10f", E_1) << std::endl;

                }
                if (world.rank() == 0) {
                    std::cout << "MP2 Energy   " << mp2 << std::endl;
                    std::cout << "CCSD Energy  " << E1 << std::endl;
                }
                return E1;
            }


            //TODO need to update equation with different options
            // ccsd energy for performance calculation
            double compute_ccsd_direct2(TArray &t1, TArray &t2) {

                auto n_occ = trange1_engine_->get_actual_occ();

                TArray g_abij = ccsd_intermediate_->get_abij();

                auto& world = g_abij.get_world();

                if(world.rank() == 0){
                    std::cout << "Use Direct CCSD Compute" <<std::endl;
                }
                TArray f_ai;
                f_ai("a,i") = fock_("a,i");

                world.gop.fence();

//      std::cout << g_abij << std::endl;

                TArray d1(f_ai.get_world(), f_ai.trange(), f_ai.get_shape(),
                           f_ai.get_pmap());
                create_d_ai(d1, orbital_energy_, n_occ);

                TArray d2(world, g_abij.trange(),
                           g_abij.get_shape(), g_abij.get_pmap());
                create_d_abij(d2, orbital_energy_, n_occ);

                t1("a,i") = f_ai("a,i") * d1("a,i");
                t2("a,b,i,j") = g_abij("a,b,i,j") * d2("a,b,i,j");

//      std::cout << t1 << std::endl;
//      std::cout << t2 << std::endl;
                TArray tau;
                tau("a,b,i,j") = t2("a,b,i,j") + t1("a,i") * t1("b,j");

                double E0 = 0.0;
                double E1 = 2.0 * TA::dot(f_ai("a,i"), t1("a,i")) +
                            TA::dot(2.0 * g_abij("a,b,i,j") - g_abij("b,a,i,j"),
                                    tau("a,b,i,j"));
                double dE = std::abs(E1 - E0);
                double mp2 = E1;
//      std::cout << E1 << std::endl;

                // get all two electron integrals
                TArray g_ijkl = ccsd_intermediate_->get_ijkl();
                TArray g_iajb = ccsd_intermediate_->get_iajb();
                TArray g_ijak = ccsd_intermediate_->get_ijak();
                TArray g_ijka = ccsd_intermediate_->get_ijka();

                // get three center integrals
                TArray Xab = ccsd_intermediate_->get_Xab();
                TArray Xij = ccsd_intermediate_->get_Xij();
                TArray Xai = ccsd_intermediate_->get_Xai();

                // get mo coefficient
                TArray ca = ccsd_intermediate_->get_Ca();
                TArray ci = ccsd_intermediate_->get_Ci();

                //optimize t1 and t2
                std::size_t iter = 0ul;
                double error = 1.0;
                TArray r1;
                TArray r2;

                if (world.rank() == 0) {
                    std::cout << "Start Iteration" << std::endl;
                };
                auto diis = get_diis(world);

                while ((dE >= 1.0e-7 || error >= 1e-7)) {

                    //start timer
                    auto time0 = mpqc_time::now();

                    TArray u2_u11;
                    // compute half transformed intermediates
                    auto tu0 = mpqc_time::now();
                    {
                        u2_u11 = ccsd_intermediate_->compute_u2_u11(t2, t1);
                    }
                    world.gop.fence();

                    if(iter == 0){
                        utility::print_size_info(u2_u11,"U_aaoo");
                    }

                    if (iter == 0 && world.rank() == 0) {
                        std::cout << "iter " << "    deltaE    " << "            residual       "
                        << "      energy     " << "    U/second  " << " total/second "<<std::endl;
                    }

                    auto tu1 = mpqc_time::now();
                    auto duration_u = mpqc_time::duration_in_s(tu0, tu1);

//                    if (g_abij.get_world().rank() == 0) {
//                        std::cout << "Time to compute U intermediates   " << duration << std::endl;
//                    }


                    // intermediates for t1
                    // external index i and a
                    TArray h_ac, h_ki, h_kc;
                    {
                        h_ac("a,c") = -(2.0 * g_abij("c,d,k,l") - g_abij("c,d,l,k")) * tau("a,d,k,l");

                        h_ki("k,i") = (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * tau("c,d,i,l");

                        h_kc("k,c") = f_ai("c,k")
                                      + (-g_abij("d,c,k,l")+ 2.0 * g_abij("c,d,k,l")) * t1("d,l");
                    }
//        g_abij.get_world().gop.fence();

                    // compute residual r1(n) = t1(n+1) - t1(n)
                    // external index i and a
                    {
                        r1("a,i") = -t1("a,i") + d1("a,i") * (
                                //
                                f_ai("a,i")
                                - 2.0 * f_ai("c,k") * t1("c,i") * t1("a,k")
                                //
                                + h_ac("a,c") * t1("c,i") - t1("a,k") * h_ki("k,i")
                                //
                                + h_kc("k,c") * (2.0 * t2("c,a,k,i") - t2("c,a,i,k")
                                                 + t1("a,k") * t1("c,i") )
                                //
                                + (2.0 * g_abij("c,a,k,i") - g_iajb("k,a,i,c")) * t1("c,k")
                                //
                                + (2.0 * u2_u11("p,r,k,i")- u2_u11("p,r,i,k")) * ci("p,k") * ca("r,a")
                                //
                                - (2.0 * g_ijak("k,l,c,i") - g_ijak("l,k,c,i")) * tau("c,a,k,l")
                        );
                    }

//        g_abij.get_world().gop.fence();
                    // intermediates for t2
                    // external index i j a b

                    TArray a_klij, b_abij, j_akic, k_kaic, T;
                    TArray g_ki, g_ac;
                    {



                        T("d,b,i,l") =
                                0.5 * t2("d,b,i,l") + t1("d,i") * t1("b,l");

                        a_klij("k,l,i,j") = g_ijkl("k,l,i,j")

                                            + g_ijka("k,l,i,c") * t1("c,j")

                                            + g_ijak("k,l,c,j") * t1("c,i")

                                            + g_abij("c,d,k,l") * tau("c,d,i,j");

                        b_abij("a,b,i,j") = (u2_u11("p,r,i,j")*ca("r,b") - ci("r,k")*t1("b,k")*u2_u11("p,r,i,j")) * ca("p,a")

                                            - u2_u11("p,r,i,j") * ci("p,k") * ca("r,b") * t1("a,k");

                        g_ki("k,i") = h_ki("k,i") + f_ai("c,k") * t1("c,i")

                                      + (2.0 * g_ijka("k,l,i,c")

                                      - g_ijka("l,k,i,c")) * t1("c,l");

                        g_ac("a,c") = h_ac("a,c") - f_ai("c,k") * t1("a,k")

                                      + (2.0*Xai("X,d,k")*t1("d,k"))*Xab("X,a,c")

                                      - (Xab("X,a,d")*t1("d,k"))*Xai("X,c,k");

                        j_akic("a,k,i,c") = g_abij("a,c,i,k")

                                            - g_ijka("l,k,i,c") * t1("a,l")

                                            + (Xab("X,a,d")*t1("d,i"))*Xai("X,c,k")

                                            - g_abij("c,d,k,l") * T("d,a,i,l")

                                            + (g_abij("c,d,k,l") - 0.5*g_abij("d,c,k,l")) * t2("a,d,i,l");

                        k_kaic("k,a,i,c") = g_iajb("k,a,i,c")

                                            - g_ijka("k,l,i,c") * t1("a,l")

                                            + (Xai("X,d,k")*t1("d,i"))*Xab("X,a,c")

                                            - g_abij("d,c,k,l") * T("d,a,i,l");

                    }

//        g_abij.get_world().gop.fence();
                    // compute residual r2(n) = t2(n+1) - t2(n)
                    {
                        r2("a,b,i,j") = -t2("a,b,i,j") + d2("a,b,i,j") * (
                                //
                                g_abij("a,b,i,j")
                                //
                                + a_klij("k,l,i,j") * tau("a,b,k,l")

                                //
                                + b_abij("a,b,i,j")

                                // permutation part
                                //
                                + (g_ac("a,c") * t2("c,b,i,j")
                                   - g_ki("k,i") * t2("a,b,k,j"))

                                + (g_ac("b,c") * t2("c,a,j,i")
                                   - g_ki("k,j") * t2("b,a,k,i"))

                                + Xab("X,b,c")*t1("c,j")*Xai("X,a,i")

                                + Xab("X,a,c")*t1("c,i")*Xai("X,b,j")

                                - g_iajb("k,b,i,c") * t1("c,j") * t1("a,k")

                                - g_iajb("k,a,j,c") * t1("c,i") * t1("b,k")
                                //
                                - (g_ijak("i,j,a,k") + g_abij("a,c,i,k") * t1("c,j")) * t1("b,k")

                                - (g_ijak("j,i,b,k") + g_abij("b,c,j,k") * t1("c,i")) * t1("a,k")

                                + (j_akic("a,k,i,c") - 0.5*k_kaic("k,a,i,c")) *
                                  (2.0 * t2("c,b,k,j") - t2("b,c,k,j"))

                                + (j_akic("b,k,j,c") - 0.5*k_kaic("k,b,j,c")) *
                                  (2.0 * t2("c,a,k,i") - t2("a,c,k,i"))

                                - 0.5 * k_kaic("k,a,i,c") * t2("b,c,k,j")
                                    - k_kaic("k,b,i,c") * t2("a,c,k,j")

                                - 0.5 * k_kaic("k,b,j,c") * t2("a,c,k,i")
                                    - k_kaic("k,a,j,c") * t2("b,c,k,i")
                        );
                    }
//        g_abij.get_world().gop.fence();

                    t1("a,i") = t1("a,i") + r1("a,i");
                    t2("a,b,i,j") = t2("a,b,i,j") + r2("a,b,i,j");
                    t1.truncate();
                    t2.truncate();

                    mpqc::cc::T1T2<double, Tile, Policy> t(t1, t2);
                    mpqc::cc::T1T2<double, Tile, Policy> r(r1, r2);
                    error = r.norm() / size(t);
                    diis.extrapolate(t, r);

                    //update t1 and t2
                    t1("a,i") = t.first("a,i");
                    t2("a,b,i,j") = t.second("a,b,i,j");

                    tau("a,b,i,j") = t2("a,b,i,j") + t1("a,i") * t1("b,j");

                    // recompute energy
                    E0 = E1;
                    E1 = 2.0 * TA::dot(f_ai("a,i"), t1("a,i"))
                         +
                         TA::dot((2.0 * g_abij("a,b,i,j") - g_abij("b,a,i,j")),
                                 tau("a,b,i,j"));
                    dE = std::abs(E0 - E1);
                    iter += 1ul;

                    auto time1 = mpqc_time::now();
                    auto duration_t = mpqc_time::duration_in_s(time0, time1);

                    if (world.rank() == 0) {
                        std::cout.precision(15);
                        std::cout<< iter << "  " << dE << "  " << error <<
                        "  " << E1 << "  " << duration_u << " " << duration_t
                        <<std::endl;
                    }

                    world.gop.fence();
//        std::cout << indent << scprintf("%-5.0f", iter) << scprintf("%-20.10f", Delta_E)
//        << scprintf("%-15.10f", E_1) << std::endl;

                }
                if (world.rank() == 0) {
                    std::cout << "MP2 Energy      " << mp2 << std::endl;
                    std::cout << "CCSD Energy     " << E1 << std::endl;
                }
                return E1;
            }

            double compute_ccsd_direct(TArray &t1, TArray &t2) {

                bool print_detail = options_.HasMember("PrintDetail") ? options_["PrintDetail"].GetBool() : false;

                auto n_occ = trange1_engine_->get_actual_occ();

                TArray g_abij = ccsd_intermediate_->get_abij();

                auto& world = g_abij.get_world();

                if(world.rank() == 0){
                    std::cout << "Use Direct CCSD Compute" <<std::endl;
                }

                TArray f_ai;
                f_ai("a,i") = fock_("a,i");

                world.gop.fence();

//      std::cout << g_abij << std::endl;

                TArray d1(f_ai.get_world(), f_ai.trange(), f_ai.get_shape(), f_ai.get_pmap());

                create_d_ai(d1, orbital_energy_, n_occ);

                t1("a,i") = f_ai("a,i") * d1("a,i");

                t2 = d_abij(g_abij,orbital_energy_,n_occ);

//      std::cout << t1 << std::endl;
//      std::cout << t2 << std::endl;
                TArray tau;
                tau("a,b,i,j") = t2("a,b,i,j") + t1("a,i") * t1("b,j");

                double E0 = 0.0;
                double E1 = 2.0 * TA::dot(f_ai("a,i"), t1("a,i")) +
                            TA::dot(2.0 * g_abij("a,b,i,j") - g_abij("b,a,i,j"),
                                    tau("a,b,i,j"));
                double dE = std::abs(E1 - E0);
                double mp2 = E1;
//      std::cout << E1 << std::endl;

                // get all two electron integrals
                TArray g_ijkl = ccsd_intermediate_->get_ijkl();
                TArray g_iajb = ccsd_intermediate_->get_iajb();
                TArray g_ijak = ccsd_intermediate_->get_ijak();
                TArray g_ijka = ccsd_intermediate_->get_ijka();

                // get three center integrals
                TArray Xab = ccsd_intermediate_->get_Xab();
                TArray Xij = ccsd_intermediate_->get_Xij();
                TArray Xai = ccsd_intermediate_->get_Xai();

                // get mo coefficient
                TArray ca = ccsd_intermediate_->get_Ca();
                TArray ci = ccsd_intermediate_->get_Ci();

                //optimize t1 and t2
                std::size_t iter = 0ul;
                double error = 1.0;
                TArray r1;
                TArray r2;


                double converge = options_.HasMember("Converge") ? options_["Converge"].GetDouble() : 1.0e-7;

                if (world.rank() == 0) {
                    std::cout << "Start Iteration" << std::endl;
                    std::cout << "Convergence " << converge << std::endl;
                };

                auto diis = get_diis(world);

                while (true){

                    //start timer
                    auto time0 = mpqc_time::now();

                    TArray::wait_for_lazy_cleanup(world);
                    TArray::wait_for_lazy_cleanup(world);

                    TArray u2_u11;
                    // compute half transformed intermediates
                    auto tu0 = mpqc_time::now();
                    {
                        u2_u11 = ccsd_intermediate_->compute_u2_u11(t2, t1);
                    }
                    world.gop.fence();

                    if(print_detail){
                        utility::print_size_info(u2_u11,"U_aaoo");
                    }
                    else if(iter == 0){
                        utility::print_size_info(u2_u11,"U_aaoo");
                    }


                    auto tu1 = mpqc_time::now();
                    auto duration_u = mpqc_time::duration_in_s(tu0, tu1);

//                    if (g_abij.get_world().rank() == 0) {
//                        std::cout << "Time to compute U intermediates   " << duration << std::endl;
//                    }


                    TArray h_ac, h_ki;
                    {
                        // intermediates for t1
                        // external index i and a

                        h_ac("a,c") = -(2.0 * g_abij("c,d,k,l") - g_abij("c,d,l,k")) * tau("a,d,k,l");

                        h_ki("k,i") = (2.0 * g_abij("c,d,k,l") - g_abij("d,c,k,l")) * tau("c,d,i,l");


                    // compute residual r1(n) = t1(n+1) - t1(n)
                    // external index i and a

                        r1("a,i") =  f_ai("a,i") - 2.0 * f_ai("c,k") * t1("c,i") * t1("a,k");

                        r1("a,i") += h_ac("a,c") * t1("c,i") - t1("a,k") * h_ki("k,i");

                        {
                            TArray h_kc;
                            h_kc("k,c") = f_ai("c,k") + (-g_abij("d,c,k,l")+ 2.0 * g_abij("c,d,k,l")) * t1("d,l");

                            r1("a,i") += h_kc("k,c") * (2.0 * t2("c,a,k,i") - t2("c,a,i,k") + t1("a,k") * t1("c,i") );
                        }

                        r1("a,i") += (2.0 * g_abij("c,a,k,i") - g_iajb("k,a,i,c")) * t1("c,k");

                        r1("a,i") += (2.0 * u2_u11("p,r,k,i")- u2_u11("p,r,i,k")) * ci("p,k") * ca("r,a");

                        r1("a,i") -= (2.0 * g_ijak("k,l,c,i") - g_ijak("l,k,c,i")) * tau("c,a,k,l");

                        r1("a,i") *= d1("a,i");

                        r1("a,i") -= t1("a,i");

                    }

                    // intermediates for t2
                    // external index i j a b
                    {

                        // permutation term
                        {
                            r2("a,b,i,j") = Xab("X,b,c")*t1("c,j")*Xai("X,a,i");

                            r2("a,b,i,j") -= g_iajb("k,b,i,c") * t1("c,j") * t1("a,k");

                            r2("a,b,i,j") -= (g_ijak("i,j,a,k") + g_abij("a,c,i,k") * t1("c,j")) * t1("b,k");
                        }

                        {
                            // intermediates g
                            TArray g_ki, g_ac;

                            g_ki("k,i") = h_ki("k,i") + f_ai("c,k") * t1("c,i") + (2.0 * g_ijka("k,l,i,c") - g_ijka("l,k,i,c")) * t1("c,l");

                            g_ac("a,c") = h_ac("a,c") - f_ai("c,k") * t1("a,k")

                                          + 2.0*Xai("X,d,k")*t1("d,k")*Xab("X,a,c")

                                          - Xab("X,a,d")*t1("d,k")*Xai("X,c,k");

                            r2("a,b,i,j") += (g_ac("a,c") * t2("c,b,i,j") - g_ki("k,i") * t2("a,b,k,j"));
                        }

                        {
                            TArray j_akic;
                            TArray k_kaic;
                            // compute j and k intermediate
                            {
                                TArray T;

                                T("d,b,i,l") = 0.5 * t2("d,b,i,l") + t1("d,i") * t1("b,l");

                                j_akic("a,k,i,c") = g_abij("a,c,i,k")

                                                    - g_ijka("l,k,i,c") * t1("a,l")

                                                    + (Xab("X,a,d")*t1("d,i"))*Xai("X,c,k")

                                                    - g_abij("c,d,k,l") * T("d,a,i,l")

                                                    + (g_abij("c,d,k,l") - 0.5*g_abij("d,c,k,l")) * t2("a,d,i,l");

                                k_kaic("k,a,i,c") = g_iajb("k,a,i,c")

                                                    - g_ijka("k,l,i,c") * t1("a,l")

                                                    + (Xai("X,d,k")*t1("d,i"))*Xab("X,a,c")

                                                    - g_abij("d,c,k,l") * T("d,a,i,l");

                                if(print_detail){
                                    utility::print_size_info(T,"T");
                                    utility::print_size_info(j_akic,"J_akic");
                                    utility::print_size_info(k_kaic,"K_kaic");
                                }
                            }

                            r2("a,b,i,j") += 0.5 * (2.0 * j_akic("a,k,i,c") - k_kaic("k,a,i,c")) * (2.0 * t2("c,b,k,j") - t2("b,c,k,j"));

                            r2("a,b,i,j") += - 0.5 * k_kaic("k,a,i,c") * t2("b,c,k,j") - k_kaic("k,b,i,c") * t2("a,c,k,j");
                        }


                        // perform permutation
                        r2("a,b,i,j") = r2("a,b,i,j") + r2("b,a,j,i");

                        r2("a,b,i,j") += g_abij("a,b,i,j");

                        {
                            // intermediate a
                            TArray a_klij;
                            a_klij("k,l,i,j") = g_ijkl("k,l,i,j")

                                                + g_ijka("k,l,i,c") * t1("c,j")

                                                + g_ijak("k,l,c,j") * t1("c,i")

                                                + g_abij("c,d,k,l") * tau("c,d,i,j");


                            r2("a,b,i,j") +=  a_klij("k,l,i,j") * tau("a,b,k,l");
                        }


                        {
                            TArray b_abij;

                            b_abij("a,b,i,j") = (u2_u11("p,r,i,j")*ca("r,b") - ci("r,k")*t1("b,k")*u2_u11("p,r,i,j")) * ca("p,a")

                                                - u2_u11("p,r,i,j") * ci("p,k") * ca("r,b") * t1("a,k");

                            r2("a,b,i,j") += b_abij("a,b,i,j");

                            if(print_detail){
                                utility::print_size_info(b_abij,"B_abij");
                            }
                        }

                        d_abij_inplace(r2, orbital_energy_, n_occ);

                        r2("a,b,i,j") -= t2("a,b,i,j");

                    }

                    t1("a,i") = t1("a,i") + r1("a,i");
                    t2("a,b,i,j") = t2("a,b,i,j") + r2("a,b,i,j");
                    t1.truncate();
                    t2.truncate();

                    // recompute energy
                    E0 = E1;
                    E1 = 2.0 * TA::dot(f_ai("a,i"), t1("a,i"))
                         +
                         TA::dot((2.0 * g_abij("a,b,i,j") - g_abij("b,a,i,j")),
                                 tau("a,b,i,j"));
                    dE = std::abs(E0 - E1);

                    if (iter == 0 && world.rank() == 0) {
                        std::cout << "iter " << "    deltaE    " << "            residual       "
                        << "      energy     " << "    U/second  " << " total/second "<<std::endl;
                    }



                    if(dE >= converge || error >= converge){

                        mpqc::cc::T1T2<double, Tile, Policy> t(t1, t2);
                        mpqc::cc::T1T2<double, Tile, Policy> r(r1, r2);
                        error = r.norm() / size(t);
                        diis.extrapolate(t, r);

                        //update t1 and t2
                        t1("a,i") = t.first("a,i");
                        t2("a,b,i,j") = t.second("a,b,i,j");

                        if(print_detail){
                            utility::print_size_info(r2,"R2");
                            utility::print_size_info(t2,"T2");
                        }

                        tau("a,b,i,j") = t2("a,b,i,j") + t1("a,i") * t1("b,j");

                        auto time1 = mpqc_time::now();
                        auto duration_t = mpqc_time::duration_in_s(time0, time1);

                        if (world.rank() == 0) {
                            std::cout.precision(15);
//                            std::cout.width(20);
                            std::cout << iter << "  " << dE << "  " << error <<
                            "  " << E1 << "  " << duration_u << " " << duration_t
                            <<std::endl;
                        }

                        iter += 1ul;
                    }
                    else{

                        auto time1 = mpqc_time::now();
                        auto duration_t = mpqc_time::duration_in_s(time0, time1);

                        if (world.rank() == 0) {
                            std::cout.precision(15);
//                            std::cout.width(20);
                            std::cout << iter << "  " << dE << "  " << error <<
                            "  " << E1 << "  " << duration_u << " " << duration_t
                            <<std::endl;
                        }

                        break;
                    }

                }
                if (world.rank() == 0) {
                    std::cout << "MP2 Energy      " << mp2 << std::endl;
                    std::cout << "CCSD Energy     " << E1 << std::endl;
                }
                return E1;
            }

        protected:
            // fock matrix
            TArray fock_;

            // orbital energy
            Eigen::VectorXd orbital_energy_;

            // TRange1 Engine class
            std::shared_ptr<mpqc::TRange1Engine> trange1_engine_;

            // CCSD intermediate
            std::shared_ptr<mpqc::cc::CCSDIntermediate<Tile, Policy>> ccsd_intermediate_;

            // option member
            rapidjson::Document options_;

        }; // class CCSD

    } //namespace cc
} //namespace mpqc


#endif //TILECLUSTERCHEM_CCSD_H
