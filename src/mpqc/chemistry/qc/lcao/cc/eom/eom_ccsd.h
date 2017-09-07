//
//
#ifndef MPQC4_SRC_MPQC_CHEMISTRY_QC_LCAO_CC_EOM_EOM_CCSD_H_
#define MPQC4_SRC_MPQC_CHEMISTRY_QC_LCAO_CC_EOM_EOM_CCSD_H_

#include "mpqc/chemistry/qc/lcao/cc/ccsd.h"
#include "mpqc/chemistry/qc/lcao/cc/ccsd_hbar.h"
#include "mpqc/chemistry/qc/lcao/ci/cis.h"
#include "mpqc/chemistry/qc/properties/excitation_energy.h"
#include "mpqc/math/linalg/davidson_diag.h"

namespace mpqc {
namespace lcao {

// close-shell eom-ccsd
template <typename Tile, typename Policy>
class EOM_CCSD : public CCSD<Tile, Policy>, public Provides<ExcitationEnergy> {
 public:
  using TArray = TA::DistArray<Tile, Policy>;
  using GuessVector = ::mpqc::cc::T1T2<TArray, TArray>;
  using numeric_type = typename Tile::numeric_type;

 public:
  // clang-format off
  /**
   * KeyVal constructor
   * @param kv
   *
   * | Keyword | Type | Default| Description |
   * |---------|------|--------|-------------|
   * | max_vector | int | 8 | max number of guess vector per root |
   * | vector_threshold | double | 1.0e-5 | threshold for the norm of new guess vector |
   *
   */

  // clang-format on
  EOM_CCSD(const KeyVal &kv) : CCSD<Tile, Policy>(kv) {
    max_vector_ = kv.value<int>("max_vector", 8);
    vector_threshold_ = kv.value<double>("vector_threshold", 1.0e-5);
  }

  void obsolete() override {
    CCSD<Tile, Policy>::obsolete();
    TArray g_ijab_ = TArray();

    TArray FAB_ = TArray();
    TArray FIJ_ = TArray();
    TArray FIA_ = TArray();

    TArray WIbAj_ = TArray();
    TArray WIbaJ_ = TArray();

    TArray WAbCd_ = TArray();
    TArray WAbCi_ = TArray();

    TArray WKlIj_ = TArray();
    TArray WKaIj_ = TArray();

    TArray WAkCd_ = TArray();
    TArray WKlIc_ = TArray();
  }

 protected:
  using CCSD<Tile, Policy>::can_evaluate;
  using CCSD<Tile, Policy>::evaluate;

  bool can_evaluate(ExcitationEnergy *ex_energy) override {
    return ex_energy->order() == 0;
  }

  void evaluate(ExcitationEnergy *ex_energy) override;

 private:
  // preconditioner in DavidsonDiag, approximate the diagonal H_bar matrix
  struct Preconditioner : public DavidsonDiagPreconditioner<GuessVector> {
    /// diagonal of F_ij matrix
    EigenVector<numeric_type> eps_o;
    /// diagonal of F_ab matrix
    EigenVector<numeric_type> eps_v;

    Preconditioner(const EigenVector<numeric_type> &eps_O,
                   const EigenVector<numeric_type> &eps_V)
        : eps_o(eps_O), eps_v(eps_V) {}

    // default constructor
    Preconditioner() : eps_o(), eps_v() {}

    virtual void compute(const numeric_type &e, GuessVector &guess) const {
      const auto &eps_v = this->eps_v;
      const auto &eps_o = this->eps_o;

      auto task1 = [&eps_v, &eps_o, e](Tile &result_tile) {
        const auto &range = result_tile.range();
        float norm = 0.0;
        for (const auto &i : range) {
          const auto result = result_tile[i] / (e + eps_o[i[1]] - eps_v[i[0]]);
          result_tile[i] = result;
          norm += result * result;
        }
        return std::sqrt(norm);
      };

      auto task2 = [&eps_v, &eps_o, e](Tile &result_tile) {
        const auto &range = result_tile.range();
        float norm = 0.0;
        for (const auto &i : range) {
          const auto result = result_tile[i] / (e - eps_v[i[0]] - eps_v[i[1]] +
                                                eps_o[i[2]] + eps_o[i[3]]);
          result_tile[i] = result;
          norm += result * result;
        }
        return std::sqrt(norm);
      };

      TA::foreach_inplace(guess.t1, task1);
      TA::foreach_inplace(guess.t2, task2);

      guess.t1.world().gop.fence();
    }
  };

  std::size_t max_vector_;   // max number of guess vector
  double vector_threshold_;  // threshold for norm of new guess vector

  TArray g_ijab_;

  // F intermediates
  TArray FAB_;
  TArray FIJ_;
  TArray FIA_;

  // W intermediates
  TArray WIbAj_;
  TArray WIbaJ_;
  TArray WAbCd_;  // this may not be initialized
  TArray WAbCi_;
  TArray WKlIj_;
  TArray WKaIj_;
  TArray WAkCd_;
  TArray WKlIc_;
  // TArray WKliC_;

  std::vector<GuessVector> C_;  // initial guess vector

  // compute F and W intermediates
  void compute_FWintermediates();

  // compute contractions of HSS, HSD, HDS, and HDD
  //                         with guess vector Ci
  // reference: CPL, 248 (1996), 189
  TArray compute_HSS_HSD_C(const TArray &Cai, const TArray &Cabij);
  TArray compute_HDS_HDD_C(const TArray &Cai, const TArray &Cabij);

  void init() {
    g_ijab_ = this->get_ijab();

    compute_FWintermediates();

    auto remove_integral = [](const Formula &formula) {
      return formula.rank() == 4;
    };

    this->lcao_factory().registry().purge_if(remove_integral);
  }

  EigenVector<numeric_type> eom_ccsd_davidson_solver(std::size_t max_iter,
                                                     double convergence);
};

#if TA_DEFAULT_POLICY == 0
extern template class EOM_CCSD<TA::TensorD, TA::DensePolicy>;
#elif TA_DEFAULT_POLICY == 1
extern template class EOM_CCSD<TA::TensorD, TA::SparsePolicy>;
#endif

}  // namespace lcao
}  // namespace mpqc

#include "eom_ccsd_impl.h"

#endif
