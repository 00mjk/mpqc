#ifndef MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_ZRHF_H_
#define MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_ZRHF_H_

#include "mpqc/chemistry/qc/lcao/basis/basis.h"
#include "mpqc/chemistry/qc/lcao/expression/trange1_engine.h"
#include "mpqc/chemistry/qc/lcao/factory/periodic_ao_factory.h"
#include "mpqc/chemistry/qc/lcao/scf/builder.h"

#include <memory>

#include <tiledarray.h>

#include "mpqc/chemistry/qc/lcao/wfn/ao_wfn.h"
#include "mpqc/chemistry/qc/properties/energy.h"
#include "mpqc/util/external/c++/memory"

namespace mpqc {
namespace lcao {

using MatrixzVec = std::vector<MatrixZ>;
using VectorzVec = std::vector<VectorZ>;
using VectordVec = std::vector<VectorD>;
using Matrix = RowMatrixXd;

/**
 * complex-valued Restricted Hartree-Fock class
 */
template <typename Tile, typename Policy>
class zRHF : public PeriodicAOWavefunction<Tile, Policy>,
             public Provides<Energy /*,
          CanonicalOrbitalSpace<TA::DistArray<TA::TensorZ, TA::SparsePolicy>>,
          PopulatedOrbitalSpace<TA::DistArray<TA::TensorZ, TA::SparsePolicy>>*/> {
 public:
	using array_type = typename PeriodicAOWavefunction<Tile, Policy>::ArrayType;
	using array_type_z = TA::DistArray<TA::TensorZ, Policy>;

  zRHF() = default;

  /**
   * KeyVal constructor for zRHF
   *
   * keywords: takes all keywords from PeriodicAOWavefunction
   *
   * | Keyword | Type | Default| Description |
   * |---------|------|--------|-------------|
   * | converge | double | 1.0e-07 | converge limit |
   * | max_iter | int | 30 | maximum number of iteration |
   * | soad_guess | bool | true | if use SOAD guess for initial Fock build |
   * | print_detail | bool | false | if print extra computation&time info |
   * | max_condition_num | double | 1.0e8 | maximum condition number for overlap
   * matrix |
   *
   */
  zRHF(const KeyVal& kv);

	~zRHF() {}

  void obsolete() override;

  /// return crystal orbital coefficients
  MatrixzVec co_coeff() override { return C_; }

  /// return crystal orbital energies
  VectordVec co_energy() override { return eps_; }

  /// return # of k points in each direction
  Vector3i nk() override { return nk_; }

  /// return the cardinal number of k points
  int64_t k_size() override { return k_size_; }

 private:
  /*!
   * \brief This performs SCF procedure for zRHF
   *
   * @param thresh the target SCF convergence threshold
   * @todo document \c thresh parameter
   * @throws MaxIterExceeded if exceeded the limit on the number of iteration
   */
  void solve(double thresh);

  /*!
   * \brief This diagonalizes Fock matrix in reciprocal space and
   * computes density: D_ = Int_k( Exp(I k.R) C(occ).C(occ)t )
   */
	array_type compute_density();

  /*!
   * \brief This transforms an integral matrix from real to reciprocal space
   * \param matrix the real-space integral matrix
   * \return the reciprocal-space integral matrix
   */
	array_type_z transform_real2recip(array_type& matrix);

  /*!
   * \brief This changes phase factor of a complex value
   * \param arg_value original complex value
   * \param factor \phi in e^(i \phi)
   */
  MatrixZ reverse_phase_factor(MatrixZ& mat0);

 protected:
	array_type S_;
	array_type D_;
  bool print_detail_;

 private:
	array_type T_;
	array_type V_;
	array_type_z Sk_;
	array_type H_;
	array_type J_;
	array_type K_;
	array_type F_;
	array_type_z Fk_;

	std::unique_ptr<scf::PeriodicFockBuilder<Tile, Policy>> f_builder_;

  MatrixzVec C_;
  VectordVec eps_;
  MatrixzVec X_;

  double energy_;
  int64_t docc_;

  const KeyVal kv_;
  int64_t maxiter_;
  double max_condition_num_;

  Vector3i R_max_;
  Vector3i RJ_max_;
  Vector3i RD_max_;
  Vector3i nk_;
  Vector3d dcell_;
  int64_t R_size_;
  int64_t RJ_size_;
  int64_t RD_size_;
  int64_t k_size_;

  double init_duration_ = 0.0;
  double j_duration_ = 0.0;
  double k_duration_ = 0.0;
  double trans_duration_ = 0.0;
  double d_duration_ = 0.0;
  double scf_duration_ = 0.0;

  /*!
   * \brief This initializes zRHF by assigning values to private members
   * and computing initial guess for the density
   *
   * \param kv KeyVal object
   */
  virtual void init(const KeyVal& kv);

  bool can_evaluate(Energy* energy) override;
  void evaluate(Energy* result) override;

  /// returns Coulomb term J_μν
	virtual array_type J_builder();
  /// returns Exchange term K_μν
	virtual array_type K_builder();
  /// returns Hartree-Fock energy
  virtual double compute_energy();
  /// initializes other stuff (may be used by derived class)
  virtual void init_other() {}
	/// initializes periodic four-center Fock builder
	virtual void init_fock_builder();
	/// builds Fock
	void build_F();
};

/*!
 * \brief DFzRHF class uses density fitting for Coulomb
 *
 * Refs: Burow, A. M.; Sierka, M.; Mohamed, F. JCP. 131, 214101 (2009)
 */
template <typename Tile, typename Policy>
class DFzRHF : public zRHF<Tile, Policy> {
 public:
	using array_type = typename zRHF<Tile, Policy>::array_type;
	using DirectTArray =
			typename PeriodicAOWavefunction<Tile, Policy>::DirectTArray;

	DFzRHF(const KeyVal& kv);

 private:
	array_type M_;         // charge matrix of product density <μ|ν>
	array_type n_;         // normalized charge vector <Κ>
	double q_;             // total charge of auxiliary basis functions
	array_type P_para_;    // projection matrix that projects X onto auxiliary
												 // charge vector
	array_type P_perp_;    // projection matrix that projects X onto the subspace
												 // orthogonal to auxiliary charge vector
	array_type V_;         // 2-center 2-electron integrals
	array_type V_perp_;    // part of 2-center 2-electron integrals that is
												 // orthogonal to auxiliary charge vector
	array_type G_;         // 3-center 2-electron direct integrals contracted with
												 // density matrix
	array_type inv_;       // A inverse where A = V_perp + P_para
	array_type identity_;  // idensity matrix
	std::vector<DirectTArray> Gamma_vec_;  // vector of 3-center 2-electron direct
																				 // integrals. vector size = RJ_size_
	array_type CD_;                        // intermediate for C_Xμν D_μν
	array_type IP_;                        // intermediate for inv_XY P_perp_YZ

	/// returns DF Coulomb term J_μν
	array_type J_builder() override;

	/// initializes necessary arrays for J builder
	void init_other() override;
};

#if TA_DEFAULT_POLICY == 0

#elif TA_DEFAULT_POLICY == 1
extern template class zRHF<TA::TensorD, TA::SparsePolicy>;
extern template class DFzRHF<TA::TensorD, TA::SparsePolicy>;
#endif

}  // namespace  lcao
}  // namespace  mpqc

#include "zrhf_impl.h"
#endif  // MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_ZRHF_H_
