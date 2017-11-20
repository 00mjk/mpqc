#ifndef MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_ZRHF_IMPL_H_
#define MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_ZRHF_IMPL_H_

#include "mpqc/chemistry/qc/lcao/scf/zrhf.h"

#include "mpqc/chemistry/qc/lcao/scf/decomposed_rij.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/periodic_cond_ortho.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/periodic_df_fock_builder.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/periodic_four_center_fock_builder.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/periodic_four_center_j_cadf_k_fock_builder.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/periodic_ri_j_cadf_k_fock_builder.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/periodic_soad.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/periodic_two_center_builder.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/util.h"

namespace mpqc {
namespace lcao {

/**
 * zRHF member functions
 */

template <typename Tile, typename Policy>
zRHF<Tile, Policy>::zRHF(const KeyVal& kv)
    : PeriodicAOWavefunction<Tile, Policy>(kv), kv_(kv) {}

template <typename Tile, typename Policy>
void zRHF<Tile, Policy>::init(const KeyVal& kv) {
  maxiter_ = kv.value<int64_t>("max_iter", 30);
  bool soad_guess = kv.value<bool>("soad_guess", true);
  print_detail_ = kv.value<bool>("print_detail", false);
  print_max_item_ = kv.value<int64_t>("print_max_item", 100);
  max_condition_num_ = kv.value<double>("max_condition_num", 1.0e8);

  auto& ao_factory = this->ao_factory();
  // retrieve world from periodic ao_factory
  auto& world = ao_factory.world();
  auto unitcell = ao_factory.unitcell();

  auto init_start = mpqc::fenced_now(world);

  ExEnv::out0() << ao_factory << std::endl;
  ExEnv::out0() << unitcell << std::endl;

  // the unit cell must be electrically neutral
  const auto charge = 0;
  const auto nelectrons = unitcell.total_atomic_number() - charge;
  if (nelectrons % 2 != 0)
    throw InputError("zRHF requires an even number of electrons", __FILE__,
                     __LINE__, "unitcell");
  docc_ = nelectrons / 2;
  dcell_ = unitcell.dcell();

  // retrieve unitcell info from periodic ao_factory
  R_max_ = ao_factory.R_max();
  RJ_max_ = ao_factory.RJ_max();
  RD_max_ = ao_factory.RD_max();
  R_size_ = ao_factory.R_size();
  RJ_size_ = ao_factory.RJ_size();
  RD_size_ = ao_factory.RD_size();

  // read # kpoints from keyval
  nk_ = decltype(nk_)(kv.value<std::array<int, 3>>("k_points").data());
  assert((nk_.array() > 0).all() &&
         "k_points cannot have zero or negative elements");
  k_size_ = 1 + detail::k_ord_idx(nk_(0) - 1, nk_(1) - 1, nk_(2) - 1, nk_);
  ExEnv::out0() << "zRHF computational parameters:" << std::endl;
  ExEnv::out0() << indent << "# of k points in each direction: ["
                << nk_.transpose() << "]" << std::endl;

  T_ = ao_factory.compute(L"<κ|T|λ>");  // Kinetic

  // Nuclear-attraction
  {
    ExEnv::out0()
        << "\nComputing Two Center Integral for Periodic System: < κ |V| λ >"
        << std::endl;
    auto t0 = mpqc::fenced_now(world);
    using Builder = scf::PeriodicTwoCenterBuilder<Tile, Policy>;
    auto basis =
        this->wfn_world()->basis_registry()->retrieve(OrbitalIndex(L"λ"));
    auto shell_pair_threshold = ao_factory.shell_pair_threshold();
    auto two_center_builder = std::make_unique<Builder>(
        world, basis, std::make_shared<const UnitCell>(unitcell), dcell_,
        R_max_, RJ_max_, R_size_, RJ_size_, shell_pair_threshold);
    V_ = two_center_builder->eval(Operator::Type::Nuclear);
    auto t1 = mpqc::fenced_now(world);
    auto dur = mpqc::duration_in_s(t0, t1);
    ExEnv::out0() << " Time: " << dur << " s" << std::endl;
  }

  S_ = ao_factory.compute(L"<κ|λ>");  // Overlap in real space
  Sk_ = transform_real2recip(S_);     // Overlap in reciprocal space
  H_("mu, nu") =
      T_("mu, nu") + V_("mu, nu");  // One-body hamiltonian in real space

  // compute density matrix using soad/core guess
  array_type F_init;
  if (!soad_guess) {
    ExEnv::out0() << "\nUsing CORE guess for initial Fock ..." << std::endl;
    F_init = H_;
  } else {
    F_init = gaussian::periodic_fock_soad(world, unitcell, H_, ao_factory);
  }

  // transform Fock from real to reciprocal space
  Fk_ = transform_real2recip(F_init);
  // compute orthogonalizer matrix
  X_ = utility::conditioned_orthogonalizer(Sk_, k_size_, max_condition_num_,
                                           print_max_item_);
  // compute guess density
  D_ = compute_density();

  init_fock_builder();

  auto init_end = mpqc::fenced_now(world);
  init_duration_ = mpqc::duration_in_s(init_start, init_end);
  ExEnv::out0() << "Periodic RHF Init Time: " << init_duration_ << " s"
                << std::endl;
}

template <typename Tile, typename Policy>
void zRHF<Tile, Policy>::solve(double thresh) {
  auto& ao_factory = this->ao_factory();
  auto& world = ao_factory.world();

  auto iter = 0;
  auto rms = 0.0;
  array_type Ddiff;
  auto converged = false;
  auto ezrhf = 0.0;
  auto ediff = 0.0;

  // compute nuclear-repulsion energy
  const auto erep = ao_factory.unitcell().nuclear_repulsion_energy(RJ_max_);
  ExEnv::out0() << "\nNuclear Repulsion Energy: " << erep << std::endl;

  do {
    auto iter_start = mpqc::fenced_now(world);

    // Save a copy of energy and density
    auto ezrhf_old = ezrhf;
    auto D_old = D_;

    if (print_detail_) {
      ExEnv::out0() << "\nIteration: " << iter << "\n";
    }

    // F = H + 2J - K
    auto f_start = mpqc::fenced_now(world);
    build_F();
    auto f_end = mpqc::fenced_now(world);

    // transform Fock from real to reciprocal space
    auto trans_start = mpqc::fenced_now(world);
    const auto fock_lattice_range = f_builder_->fock_lattice_range();
    Fk_ = transform_real2recip(F_, fock_lattice_range, nk_);
    auto trans_end = mpqc::fenced_now(world);
    trans_duration_ += mpqc::duration_in_s(trans_start, trans_end);

    // compute zRHF energy
    ezrhf = compute_energy();

    // compute new density
    auto d_start = mpqc::fenced_now(world);
    D_ = compute_density();
    // update density in periodic ao_factory
    ao_factory.set_density(D_);
    auto d_end = mpqc::fenced_now(world);
    d_duration_ += mpqc::duration_in_s(d_start, d_end);

    // compute difference with last iteration
    ediff = ezrhf - ezrhf_old;
    Ddiff("mu, nu") = D_("mu, nu") - D_old("mu, nu");
    auto volume = Ddiff.trange().elements_range().volume();
    rms = Ddiff("mu, nu").norm() / volume;

    if ((rms <= thresh) || fabs(ediff) <= thresh) converged = true;

    auto iter_end = mpqc::fenced_now(world);
    auto iter_duration = mpqc::duration_in_s(iter_start, iter_end);
    scf_duration_ += iter_duration;

    // Print out information
    if (print_detail_) {
      if (world.rank() == 0) {
        std::cout << "\nzRHF Energy: " << ezrhf << "\n"
                  << "Total Energy: " << ezrhf + erep << "\n"
                  << "Delta(E): " << ediff << "\n"
                  << "RMS(D): " << rms << "\n"
                  << "Fock Build Time: " << mpqc::duration_in_s(f_start, f_end)
                  << " s\n"
                  << "Transform Fock (Real->Recip) Time: "
                  << mpqc::duration_in_s(trans_start, trans_end) << " s\n"
                  << "Density Time: " << mpqc::duration_in_s(d_start, d_end)
                  << " s\n"
                  << "Iteration Time: " << iter_duration << " s\n";
      }
    } else {
      std::string niter = "Iter", nEle = "E(HF)", nTot = "E(tot)",
                  nDel = "Delta(E)", nRMS = "RMS(D)", nT = "Time(s)";
      if (iter == 0)
        ExEnv::out0() << mpqc::printf("\n\n %4s %20s %20s %20s %20s %20s\n",
                                      niter.c_str(), nEle.c_str(), nTot.c_str(),
                                      nDel.c_str(), nRMS.c_str(), nT.c_str());
      ExEnv::out0() << mpqc::printf(
          " %4d %20.12f %20.12f %20.12f %20.12f %20.3f\n", iter, ezrhf,
          ezrhf + erep, ediff, rms, iter_duration);
    }
    ++iter;

  } while ((iter < maxiter_) && (!converged));

  // save total energy to energy no matter if zRHF converges
  energy_ = ezrhf + erep;

  if (!converged) {
    // TODO read a keyval value to determine
    if (1)
      ExEnv::out0() << "\nzRHF: SCF did not converge!\n\n";
    else
      throw MaxIterExceeded("zRHF: SCF did not converge", __FILE__, __LINE__,
                            maxiter_);
  } else {
    ExEnv::out0() << "\nPeriodic Hartree-Fock iterations have converged!\n";
  }

  // store fock matrix in registry
  auto& registry = this->ao_factory().registry();
  f_builder_->register_fock(F_, registry);

  if (world.rank() == 0) {
    std::cout << "\nTotal Periodic Hartree-Fock energy = " << energy_
              << std::endl;

    if (print_detail_ && k_size_ < print_max_item_) {
      Eigen::IOFormat fmt(5);
      std::cout << "\n k | orbital energies" << std::endl;
      for (auto k = 0; k < k_size_; ++k) {
        std::cout << k << " | " << eps_[k].transpose().format(fmt) << std::endl;
      }
    }

    // print out timings
    std::cout << mpqc::printf("\nTime(s):\n");
    std::cout << mpqc::printf("\tInit:                %20.3f\n",
                              init_duration_);
    std::cout << mpqc::printf("\tCoulomb term:        %20.3f\n", j_duration_);
    std::cout << mpqc::printf("\tExchange term:       %20.3f\n", k_duration_);
    std::cout << mpqc::printf("\tReal->Recip trans:   %20.3f\n",
                              trans_duration_);
    std::cout << mpqc::printf("\tDiag + Density:      %20.3f\n", d_duration_);
    std::cout << mpqc::printf("\tTotal:               %20.3f\n\n",
                              scf_duration_);
  }
}

template <typename Tile, typename Policy>
typename zRHF<Tile, Policy>::array_type zRHF<Tile, Policy>::compute_density() {
  auto& ao_factory = this->ao_factory();
  auto& world = ao_factory.world();

  eps_.resize(k_size_);
  C_.resize(k_size_);

  auto tr0 = Fk_.trange().data()[0];
  using ::mpqc::lcao::detail::extend_trange1;
  auto tr1 = extend_trange1(tr0, RD_size_);

  auto fock_eig = array_ops::array_to_eigen(Fk_);
  for (auto k = 0; k < k_size_; ++k) {
    // Get orthogonalizer
    auto X = X_[k];
    // Symmetrize Fock
    auto F = fock_eig.block(0, k * tr0.extent(), tr0.extent(), tr0.extent());
    MatrixZ F_twice = F + F.transpose().conjugate();
    // When k=0 (gamma point), reverse phase factor of complex values
    if (k_size_ > 1 && k_size_ % 2 == 1 && k == ((k_size_ - 1) / 2))
      F_twice = reverse_phase_factor(F_twice);
    F = 0.5 * F_twice;

    // Orthogonalize Fock matrix: F' = Xt * F * X
    MatrixZ Xt = X.transpose().conjugate();
    auto XtF = Xt * F;
    auto Ft = XtF * X;

    // Diagonalize F'
    Eigen::SelfAdjointEigenSolver<MatrixZ> comp_eig_solver(Ft);
    eps_[k] = comp_eig_solver.eigenvalues();
    MatrixZ Ctemp = comp_eig_solver.eigenvectors();

    // When k=0 (gamma point), reverse phase factor of complex eigenvectors
    if (k_size_ > 1 && k_size_ % 2 == 1 && k == ((k_size_ - 1) / 2))
      Ctemp = reverse_phase_factor(Ctemp);

    C_[k] = X * Ctemp;
  }

  Matrix result_eig(tr0.extent(), tr1.extent());
  result_eig.setZero();
  for (auto R = 0; R < RD_size_; ++R) {
    auto vec_R = detail::direct_vector(R, RD_max_, dcell_);
    for (auto k = 0; k < k_size_; ++k) {
      auto vec_k = detail::k_vector(k, nk_, dcell_);
      auto C_occ = C_[k].leftCols(docc_);
      auto D_real = C_occ.conjugate() * C_occ.transpose();
      auto exponent =
          std::exp(I * vec_k.dot(vec_R)) / double(nk_(0) * nk_(1) * nk_(2));
      auto D_comp = exponent * D_real;
      result_eig.block(0, R * tr0.extent(), tr0.extent(), tr0.extent()) +=
          D_comp.real();
    }
  }

  auto result = array_ops::eigen_to_array<Tile, TA::SparsePolicy>(
      world, result_eig, tr0, tr1);
  return result;
}

template <typename Tile, typename Policy>
typename zRHF<Tile, Policy>::array_type_z
zRHF<Tile, Policy>::transform_real2recip(const array_type& matrix,
                                         const Vector3i& real_lattice_range,
                                         const Vector3i& recip_lattice_range) {
  // Make sure range values are all positive
  assert((real_lattice_range.array() >= 0).all() &&
         (recip_lattice_range.array() > 0).all());

  const auto real_lattice_size =
      1 + detail::direct_ord_idx(real_lattice_range, real_lattice_range);
  const Vector3i k_end_3D_idx = (recip_lattice_range.array() - 1).matrix();
  const auto recip_lattice_size =
      1 + detail::k_ord_idx(k_end_3D_idx, recip_lattice_range);
  const auto tiles_range = matrix.trange().tiles_range();
  assert(tiles_range.extent(1) % tiles_range.extent(0) == 0);
  assert(real_lattice_size == tiles_range.extent(1) / tiles_range.extent(0));

  array_type_z result;
  auto tr0 = matrix.trange().data()[0];
  auto tr1 = detail::extend_trange1(tr0, recip_lattice_size);
  auto& world = matrix.world();

  // Perform real->reciprocal transformation with Eigen
  // TODO: perform it with TA
  auto matrix_eig = array_ops::array_to_eigen(matrix);
  MatrixZ result_eig(tr0.extent(), tr1.extent());
  result_eig.setZero();

  auto threshold = std::numeric_limits<double>::epsilon();
  for (auto R = 0; R < real_lattice_size; ++R) {
    auto bmat =
        matrix_eig.block(0, R * tr0.extent(), tr0.extent(), tr0.extent());
    if (bmat.norm() < bmat.size() * threshold)
      continue;
    else {
      auto vec_R = detail::direct_vector(R, real_lattice_range, dcell_);
      for (auto k = 0; k < recip_lattice_size; ++k) {
        auto vec_k = detail::k_vector(k, recip_lattice_range, dcell_);
        auto exponent = std::exp(I * vec_k.dot(vec_R));
        result_eig.block(0, k * tr0.extent(), tr0.extent(), tr0.extent()) +=
            bmat * exponent;
      }
    }
  }

  result = array_ops::eigen_to_array<TA::TensorZ, TA::SparsePolicy>(
      world, result_eig, tr0, tr1);

  return result;
}

template <typename Tile, typename Policy>
typename zRHF<Tile, Policy>::array_type_z
zRHF<Tile, Policy>::transform_real2recip(const array_type& matrix) {
  const Vector3i k_end_3D_idx = (nk_.array() - 1).matrix();
  assert(k_size_ == 1 + detail::k_ord_idx(k_end_3D_idx, nk_));
  assert(R_size_ == 1 + detail::direct_ord_idx(R_max_, R_max_));
  return transform_real2recip(matrix, R_max_, nk_);
}

template <typename Tile, typename Policy>
MatrixZ zRHF<Tile, Policy>::reverse_phase_factor(MatrixZ& mat0) {
  MatrixZ result(mat0);

  for (auto row = 0; row < mat0.rows(); ++row) {
    for (auto col = 0; col < mat0.cols(); ++col) {
      std::complex<double> comp0 = mat0(row, col);

      double norm = std::abs(comp0);
      if (norm == 0.0) {
        result(row, col) = comp0;
      } else {
        double real = comp0.real();
        double imag = comp0.imag();

        double phi = std::atan(imag / real);

        double R;
        if (std::cos(phi) != 0.0) {
          R = real / std::cos(phi);
        } else {
          R = imag / std::sin(phi);
        }

        std::complex<double> comp1 = comp0 * std::exp(-1.0 * I * phi);

        result(row, col) = comp1;
      }
    }
  }

  return result;
}

template <typename Tile, typename Policy>
void zRHF<Tile, Policy>::obsolete() {
  Wavefunction::obsolete();
}

template <typename Tile, typename Policy>
bool zRHF<Tile, Policy>::can_evaluate(Energy* energy) {
  // can only evaluate the energy
  return energy->order() == 0;
}

template <typename Tile, typename Policy>
void zRHF<Tile, Policy>::evaluate(Energy* result) {
  if (!this->computed()) {
    init(kv_);
    solve(result->target_precision(0));
    this->computed_ = true;
    set_value(result, energy_);
  }
}

template <typename Tile, typename Policy>
double zRHF<Tile, Policy>::compute_energy() {
  array_type H_plus_F;
  const auto fock_lattice_range = f_builder_->fock_lattice_range();
  H_plus_F = ::mpqc::pbc::detail::add(H_, F_, R_max_, fock_lattice_range);
  Vector3i HpF_lattice_range;
  if (R_max_(0) >= fock_lattice_range(0) &&
      R_max_(1) >= fock_lattice_range(1) &&
      R_max_(2) >= fock_lattice_range(2)) {
    HpF_lattice_range = R_max_;
  } else if (R_max_(0) <= fock_lattice_range(0) &&
             R_max_(1) <= fock_lattice_range(1) &&
             R_max_(2) <= fock_lattice_range(2)) {
    HpF_lattice_range = fock_lattice_range;
  } else {
    ExEnv::out0() << "\nLattice range of H: " << R_max_.transpose()
                  << "\nLattice range of Fock: "
                  << fock_lattice_range.transpose() << std::endl;
    throw "Invalid lattice ranges!";
  }
  auto ezrhf = ::mpqc::pbc::detail::dot_product(H_plus_F, D_, HpF_lattice_range,
                                                RD_max_);
  return ezrhf;
}

template <typename Tile, typename Policy>
void zRHF<Tile, Policy>::init_fock_builder() {
  using Builder = scf::ReferencePeriodicFourCenterFockBuilder<
      Tile, Policy, zRHF<Tile, Policy>::factory_type>;
  this->f_builder_ = std::make_unique<Builder>(this->ao_factory());
}

template <typename Tile, typename Policy>
void zRHF<Tile, Policy>::build_F() {
  auto G = f_builder_->operator()(D_);
  const auto fock_lattice_range = f_builder_->fock_lattice_range();
  F_ = ::mpqc::pbc::detail::add(H_, G, R_max_, fock_lattice_range);
}

/**
 *  DFzRHF member functions
 */

template <typename Tile, typename Policy>
DFzRHF<Tile, Policy>::DFzRHF(const KeyVal& kv) : zRHF<Tile, Policy>(kv) {}

template <typename Tile, typename Policy>
void DFzRHF<Tile, Policy>::init_fock_builder() {
  using Builder =
      scf::PeriodicDFFockBuilder<Tile, Policy,
                                 DFzRHF<Tile, Policy>::factory_type>;
  this->f_builder_ = std::make_unique<Builder>(this->ao_factory());
}

/**
 *  FourCenterzRHF member functions
 */

template <typename Tile, typename Policy>
FourCenterzRHF<Tile, Policy>::FourCenterzRHF(const KeyVal& kv)
    : zRHF<Tile, Policy>(kv) {}

template <typename Tile, typename Policy>
void FourCenterzRHF<Tile, Policy>::init_fock_builder() {
  using Builder = scf::PeriodicFourCenterFockBuilder<Tile, Policy>;
  this->f_builder_ = std::make_unique<Builder>(this->ao_factory(), true, true);
}

/**
 *  RIJCADFKzRHF member functions
 */

template <typename Tile, typename Policy>
RIJCADFKzRHF<Tile, Policy>::RIJCADFKzRHF(const KeyVal& kv)
    : zRHF<Tile, Policy>(kv) {
  force_shape_threshold_ = kv.value<double>("force_shape_threshold", 0.0);
}

template <typename Tile, typename Policy>
void RIJCADFKzRHF<Tile, Policy>::init_fock_builder() {
  using Builder = scf::PeriodicRIJCADFKFockBuilder<
      Tile, Policy, RIJCADFKzRHF<Tile, Policy>::factory_type>;
  this->f_builder_ =
      std::make_unique<Builder>(this->ao_factory(), force_shape_threshold_);
}

/**
 *  FourCenterJCADFKzRHF member functions
 */

template <typename Tile, typename Policy>
FourCenterJCADFKzRHF<Tile, Policy>::FourCenterJCADFKzRHF(const KeyVal& kv)
    : zRHF<Tile, Policy>(kv) {
  force_shape_threshold_ = kv.value<double>("force_shape_threshold", 0.0);
}

template <typename Tile, typename Policy>
void FourCenterJCADFKzRHF<Tile, Policy>::init_fock_builder() {
  using Builder = scf::PeriodicFourCenterJCADFKFockBuilder<
      Tile, Policy, FourCenterJCADFKzRHF<Tile, Policy>::factory_type>;
  this->f_builder_ =
      std::make_unique<Builder>(this->ao_factory(), force_shape_threshold_);
}

}  // namespace lcao
}  // namespace mpqc

#endif  // MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_ZRHF_IMPL_H_