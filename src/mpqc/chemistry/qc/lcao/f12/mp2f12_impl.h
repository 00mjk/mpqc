//
// Created by Chong Peng on 12/6/16.
//

#ifndef SRC_MPQC_CHEMISTRY_QC_F12_MP2F12_IMPL_H_
#define SRC_MPQC_CHEMISTRY_QC_F12_MP2F12_IMPL_H_

#include "mpqc/chemistry/qc/lcao/mbpt/denom.h"

namespace mpqc {
namespace lcao {

///
/// Member functions for RMP2F12 class
///

template <typename Tile>
RMP2F12<Tile>::RMP2F12(const KeyVal& kv)
    : LCAOWavefunction<Tile, TA::SparsePolicy>(kv) {
  if (kv.exists("ref")) {
    ref_wfn_ = kv.class_ptr<Wavefunction>("ref");
  } else {
    throw std::invalid_argument(
        "Default Ref Wfn in RMP2F12 is not support! \n");
  }

  approximation_ = kv.value<char>("approximation", 'C');
  if (approximation_ != 'C' && approximation_ != 'D') {
    throw std::invalid_argument("Only approximation C or D is supported!");
  }

  cabs_singles_ = kv.value<bool>("cabs_singles", true);
}

template <typename Tile>
void RMP2F12<Tile>::obsolete() {
  mp2_corr_energy_ = 0.0;
  mp2_f12_energy_ = 0.0;
  singles_energy_ = 0.0;
  LCAOWavefunction<Tile, TA::SparsePolicy>::obsolete();
  ref_wfn_->obsolete();
}

template <typename Tile>
bool RMP2F12<Tile>::can_evaluate(Energy* energy) {
  // can only evaluate the energy
  return energy->order() == 0;
}

template <typename Tile>
void RMP2F12<Tile>::evaluate(Energy* result) {
  auto target_precision = result->target_precision(0);
  if (!this->computed() || computed_precision_ > target_precision) {
    // compute reference to higher precision than required of correlation energy
    auto target_ref_precision = target_precision / 100.;
    auto ref_energy = std::make_shared<Energy>(ref_wfn_, target_ref_precision);
    ::mpqc::evaluate(*ref_energy, ref_wfn_);

    auto& world = this->wfn_world()->world();
    auto time1 = mpqc::fenced_now(world);

    // initialize
    init(target_ref_precision);

    // compute
    RowMatrix<double> mp2_eij, f12_eij;
    std::tie(mp2_eij, f12_eij) = compute();

    auto nocc = this->trange1_engine()->get_active_occ();
    ExEnv::out0() << printf(
        "  i0     i1       eij(mp2)        eij(f12)      eij(mp2-f12) \n"
        "====== ====== =============== =============== ===============\n");
    for (int i = 0; i != nocc; ++i)
      for (int j = i; j != nocc; ++j)
        ExEnv::out0() << printf("%4d   %4d   %15.12lf %15.12lf %15.12lf\n", i,
                                j, mp2_eij(i, j), f12_eij(i, j),
                                mp2_eij(i, j) + f12_eij(i, j));

    mp2_corr_energy_ = mp2_eij.sum();
    mp2_f12_energy_ = f12_eij.sum();
    utility::print_par(world, "E_MP2: ", mp2_corr_energy_, "\n");
    utility::print_par(world, "E_F12: ", mp2_f12_energy_, "\n");

    if (cabs_singles_) {
      singles_energy_ = compute_cabs_singles();
    }

    utility::print_par(world, "E_S: ", singles_energy_, "\n");

    double energy = ref_energy->energy() + mp2_corr_energy_ + mp2_f12_energy_ +
                    singles_energy_;

    this->computed_ = true;
    this->set_value(result, energy);

    auto time2 = mpqc::fenced_now(world);
    auto time = mpqc::duration_in_s(time1, time2);
    utility::print_par(world, "F12 Time in MP2F12: ", time, " S \n");
  }
}

template <typename Tile>
std::tuple<RowMatrix<double>, RowMatrix<double>> RMP2F12<Tile>::compute() {
  auto& lcao_factory = to_lcao_factory(this->lcao_factory());
  auto& world = lcao_factory.world();

  utility::print_par(world, "\n Computing MP2F12 ", approximation_,
                     " Approximation \n");

  RowMatrix<double> Eij_MP2, Eij_F12;

  auto n_active_occ = this->trange1_engine()->get_active_occ();
  auto n_occ = this->trange1_engine()->get_occ();
  auto n_frozen = this->trange1_engine()->get_nfrozen();

  auto ijij_ijji_shape = ijij_ijji_shape_;

  // compute B term
  {
    TA::DistArray<Tile, TA::SparsePolicy> B_ijij_ijji = compute_B();
    RowMatrix<double> Eij_b =
        B_ijij_ijji("i1,j1,i2,j2")
            .reduce(f12::F12PairEnergyReductor<Tile>(
                f12::CC_ijij_bar, f12::CC_ijji_bar, n_active_occ));
    utility::print_par(world, "E_B: ", Eij_b.sum(), "\n");
    Eij_F12 = Eij_b;
  }

  // compute X term
  {
    TA::DistArray<Tile, TA::SparsePolicy> X_ijij_ijji = compute_X();
    lcao_factory.purge_operator(L"R2");

    RowMatrix<double> Eij_x =
        X_ijij_ijji("i1,j1,i2,j2")
            .reduce(f12::F12PairEnergyReductor<Tile>(
                f12::CC_ijij_bar, f12::CC_ijji_bar, n_active_occ));
    Eij_x *= -1.0;
    utility::print_par(world, "E_X: ", Eij_x.sum(), "\n");
    Eij_F12 += Eij_x;
  }

  // compute V term
  {
    TA::DistArray<Tile, TA::SparsePolicy> V_ijij_ijji = compute_V();
    // G integral in MO not needed, still need G integral in AO to compute F, K,
    // hJ
    lcao_factory.registry().purge_operator(L"G");
    lcao_factory.purge_operator(L"GR");

    // contribution from V_ijij_ijji
    // NB factor of 2 from the Hylleraas functional
    RowMatrix<double> e_ij =
        V_ijij_ijji("i1,j1,i2,j2")
            .reduce(f12::F12PairEnergyReductor<Tile>(
                2 * f12::C_ijij_bar, 2 * f12::C_ijji_bar, n_active_occ));
    Eij_F12 += e_ij;
    utility::print_par(world, "E_V: ", e_ij.sum(), "\n");
  }

  TA::DistArray<Tile, TA::SparsePolicy> t2;
  {
    TA::DistArray<Tile, TA::SparsePolicy> g_abij;
    std::tie(t2, g_abij) = compute_T();

    // compute MP2 energy and pair energies
    TA::DistArray<Tile, TA::SparsePolicy> TG_ijij_ijji;
    TG_ijij_ijji("i1,j1,i2,j2") =
        (t2("a,b,i1,j1") * g_abij("a,b,i2,j2")).set_shape(ijij_ijji_shape);
    Eij_MP2 =
        TG_ijij_ijji("i1,j1,i2,j2")
            .reduce(f12::F12PairEnergyReductor<Tile>(2, -1, n_active_occ));
  }

  // compute C term
  TA::DistArray<Tile, TA::SparsePolicy> C_ijab = compute_C();

  {
    utility::print_par(world, "Compute CT \n");
    TA::DistArray<Tile, TA::SparsePolicy> CT;
    CT("i1,j1,i2,j2") =
        (C_ijab("i1,j1,a,b") * t2("a,b,i2,j2")).set_shape(ijij_ijji_shape);

    // NB factor of 2 from the Hylleraas functional
    RowMatrix<double> Eij_ct =
        CT("i1,j1,i2,j2")
            .reduce(f12::F12PairEnergyReductor<Tile>(
                2 * f12::C_ijij_bar, 2 * f12::C_ijji_bar, n_active_occ));
    utility::print_par(world, "E_CT: ", Eij_ct.sum(), "\n");
    Eij_F12 += Eij_ct;
  }

  {
    utility::print_par(world, "Compute CC Term \n");
    TA::DistArray<Tile, TA::SparsePolicy> CC;
    auto C_bar_ijab =
        f12::convert_C_ijab(C_ijab, n_occ, n_frozen, *(this->orbital_energy()));
    CC("i1,j1,i2,j2") = (C_ijab("i1,j1,a,b") * C_bar_ijab("i2,j2,a,b"))
                            .set_shape(ijij_ijji_shape);

    RowMatrix<double> Eij_cc =
        CC("i1,j1,i2,j2")
            .reduce(f12::F12PairEnergyReductor<Tile>(
                f12::CC_ijij_bar, f12::CC_ijji_bar, n_active_occ));
    utility::print_par(world, "E_CC: ", Eij_cc.sum(), "\n");
    Eij_F12 += Eij_cc;
  }

  return std::make_tuple(Eij_MP2, Eij_F12);
}

template <typename Tile>
void RMP2F12<Tile>::init(double ref_precision) {
  this->init_sdref(ref_wfn_, ref_precision);

  this->f_pq_diagonal_ =
      make_diagonal_fpq(this->lcao_factory(), this->ao_factory(),false);

  // create shape
  auto occ_tr1 = this->trange1_engine()->get_active_occ_tr1();
  TiledArray::TiledRange occ4_trange({occ_tr1, occ_tr1, occ_tr1, occ_tr1});
  ijij_ijji_shape_ = f12::make_ijij_ijji_shape(occ4_trange);

  // initialize cabs
  closed_shell_cabs_mo_build_svd(to_ao_factory(this->ao_factory()),
                                 this->trange1_engine(), this->unocc_block());
}

template <typename Tile>
TA::DistArray<Tile, TA::SparsePolicy> RMP2F12<Tile>::compute_B() {
  TA::DistArray<Tile, TA::SparsePolicy> B;
  if (approximation_ == 'C') {
    B = f12::compute_B_ijij_ijji_C(to_lcao_factory(this->lcao_factory()),
                                   ijij_ijji_shape_);
  } else if (approximation_ == 'D') {
    B = f12::compute_B_ijij_ijji_D(to_lcao_factory(this->lcao_factory()),
                                   ijij_ijji_shape_);
  }
  return B;
}

template <typename Tile>
TA::DistArray<Tile, TA::SparsePolicy> RMP2F12<Tile>::compute_C() {
  return f12::compute_C_ijab(to_lcao_factory(this->lcao_factory()));
}

template <typename Tile>
TA::DistArray<Tile, TA::SparsePolicy> RMP2F12<Tile>::compute_V() {
  return f12::compute_V_ijij_ijji(to_lcao_factory(this->lcao_factory()),
                                  ijij_ijji_shape_);
}

template <typename Tile>
TA::DistArray<Tile, TA::SparsePolicy> RMP2F12<Tile>::compute_X() {
  TA::DistArray<Tile, TA::SparsePolicy> X = f12::compute_X_ijij_ijji(
      to_lcao_factory(this->lcao_factory()), ijij_ijji_shape_);
  auto Fij = this->lcao_factory().compute(L"(i|F|j)");
  auto Fij_eigen = math::array_to_eigen(Fij);
  f12::convert_X_ijkl(X, Fij_eigen);
  return X;
}

template <typename Tile>
std::tuple<TA::DistArray<Tile, TA::SparsePolicy>,
           TA::DistArray<Tile, TA::SparsePolicy>>
RMP2F12<Tile>::compute_T() {
  TA::DistArray<Tile, TA::SparsePolicy> g_abij, t2;
  g_abij("a,b,i,j") = this->lcao_factory().compute(L"<i j|G|a b>")("i,j,a,b");
  t2 = detail::d_abij(g_abij, *(this->orbital_energy()),
              this->trange1_engine()->get_occ(),
              this->trange1_engine()->get_nfrozen());

  return std::tuple<TA::DistArray<Tile, TA::SparsePolicy>,
                    TA::DistArray<Tile, TA::SparsePolicy>>(t2, g_abij);
}

template <typename Tile>
double RMP2F12<Tile>::compute_cabs_singles() {
  double es;
  CABSSingles<Tile> cabs_singles(to_lcao_factory(this->lcao_factory()));

  if (approximation_ == 'D') {
    es = cabs_singles.compute(false, true, true);
  } else {
    es = cabs_singles.compute(false, false, true);
  }

  return es;
}

///
/// Memeber functions for RIRMP2F12
///
///

template <typename Tile>
RIRMP2F12<Tile>::RIRMP2F12(const KeyVal& kv) : RMP2F12<Tile>(kv) {}


template <typename Tile>
void RIRMP2F12<Tile>::init(double ref_precision) {
  this->init_sdref(this->ref_wfn_, ref_precision);

  this->f_pq_diagonal_ =
      make_diagonal_fpq(this->lcao_factory(), this->ao_factory(),true);

  // create shape
  auto occ_tr1 = this->trange1_engine()->get_active_occ_tr1();
  TiledArray::TiledRange occ4_trange({occ_tr1, occ_tr1, occ_tr1, occ_tr1});
  this->ijij_ijji_shape_ = f12::make_ijij_ijji_shape(occ4_trange);

  // initialize cabs
  closed_shell_cabs_mo_build_svd(to_ao_factory(this->ao_factory()),
                                 this->trange1_engine(), this->unocc_block());
}

template <typename Tile>
TA::DistArray<Tile, TA::SparsePolicy> RIRMP2F12<Tile>::compute_B() {
  TA::DistArray<Tile, TA::SparsePolicy> B;
  if (this->approximation_ == 'C') {
    B = f12::compute_B_ijij_ijji_C_df(this->lcao_factory(),this->ao_factory(),
                                      this->ijij_ijji_shape_);
  } else if (this->approximation_ == 'D') {
    B = f12::compute_B_ijij_ijji_D_df(this->lcao_factory(), this->ao_factory(),
                                      this->ijij_ijji_shape_);
  }
  return B;
}

template <typename Tile>
TA::DistArray<Tile, TA::SparsePolicy> RIRMP2F12<Tile>::compute_C() {
  return f12::compute_C_ijab_df(this->lcao_factory());
}

template <typename Tile>
TA::DistArray<Tile, TA::SparsePolicy> RIRMP2F12<Tile>::compute_V() {
  return f12::compute_V_ijij_ijji_df(this->lcao_factory(),this->ao_factory(),
                                     this->ijij_ijji_shape_);
}

template <typename Tile>
TA::DistArray<Tile, TA::SparsePolicy> RIRMP2F12<Tile>::compute_X() {
  TA::DistArray<Tile, TA::SparsePolicy> X = f12::compute_X_ijij_ijji_df(
      this->lcao_factory(), this->ao_factory(), this->ijij_ijji_shape_);
  auto Fij = this->lcao_factory().compute(L"(i|F|j)[df]");
  auto Fij_eigen = math::array_to_eigen(Fij);
  f12::convert_X_ijkl(X, Fij_eigen);
  return X;
}

template <typename Tile>
std::tuple<TA::DistArray<Tile, TA::SparsePolicy>,
           TA::DistArray<Tile, TA::SparsePolicy>>
RIRMP2F12<Tile>::compute_T() {
  TA::DistArray<Tile, TA::SparsePolicy> g_abij, t2;
  g_abij("a,b,i,j") =
      this->lcao_factory().compute(L"<i j|G|a b>[df]")("i,j,a,b");
  t2 = detail::d_abij(g_abij, *(this->orbital_energy()),
              this->trange1_engine()->get_occ(),
              this->trange1_engine()->get_nfrozen());

  return std::tuple<TA::DistArray<Tile, TA::SparsePolicy>,
                    TA::DistArray<Tile, TA::SparsePolicy>>(t2, g_abij);
}

template <typename Tile>
double RIRMP2F12<Tile>::compute_cabs_singles() {
  double es;
  CABSSingles<Tile> cabs_singles(to_lcao_factory(this->lcao_factory()));

  if (this->approximation_ == 'D') {
    es = cabs_singles.compute(true, true, true);
  } else {
    es = cabs_singles.compute(true, false, true);
  }

  return es;
}

}  // namespace lcao
}  // namespace mpqc

#endif  // SRC_MPQC_CHEMISTRY_QC_F12_MP2F12_IMPL_H_
