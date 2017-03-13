/*
 * Created by Drew Lewis on 02/22/2017
 */

#ifndef MPQC_CHEMISTRY_QC_LCAO_INTEGRALS_DENSITYFITTING_CADF_COEFFS_H
#define MPQC_CHEMISTRY_QC_LCAO_INTEGRALS_DENSITYFITTING_CADF_COEFFS_H

#include "mpqc/chemistry/qc/lcao/basis/basis.h"
#include "mpqc/chemistry/qc/lcao/integrals/direct_task_integrals.h"
#include "mpqc/chemistry/qc/lcao/integrals/integrals.h"
#include "mpqc/chemistry/qc/lcao/integrals/screening/schwarz_screen.h"
#include "mpqc/chemistry/qc/lcao/integrals/task_integrals.h"

#include "mpqc/math/external/eigen/eigen.h"
#include "mpqc/math/linalg/sqrt_inv.h"
#include "mpqc/util/meta/make_array.h"

#include <tiledarray.h>

#include <fstream>
#include <utility>

namespace mpqc {
namespace lcao {

namespace detail {
// Function that creates a by atom (center) basis
gaussian::Basis by_center_basis(gaussian::Basis const &in);

// Create the by atom trange for the CADF coefficients
TA::TiledRange cadf_trange(gaussian::Basis const &obs_by_atom,
                           gaussian::Basis const &dfbs_by_atom);

// Computes the correct shape for the cadf by atom tensor
TA::SparseShape<float> cadf_shape(madness::World &world,
                                  TA::TiledRange const &trange);

// Print the shape of a tensor in matrix form
void print_shape(TA::Tensor<float> const &t, std::string const &file_name);

// Creates a shape for the by cluster tensor and also a list of atom tiles in
// each cluster tile
TA::SparseShape<float> cadf_shape_cluster(
    TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> const &C_atom,
    TA::TiledRange const &trange,
    std::unordered_map<int64_t, std::vector<int64_t>> &c2a  // cluster to atom
    );

// Function to compute the CADF coefficients in a by atom fashion
template <typename Array, typename DirectArray>
TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> cadf_by_atom_array(
    Array const &, DirectArray const &, TA::TiledRange const &);

// Function to compute the CADF coefficients in a by atom fashion
TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> cadf_by_atom_coeffs(
    madness::World &world, gaussian::Basis const &by_cluster_obs,
    gaussian::Basis const &by_cluster_dfbs);

// Function to actually construct the cadf by atom array
template <typename Array, typename DirectArray>
TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> cadf_by_atom_array(
    Array const &, DirectArray const &, TA::TiledRange const &);

template <typename Array>
TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> reblock_atom_to_clusters(
    Array const &, gaussian::Basis const &, gaussian::Basis const &);

// Function to create the iii tiles of the coefficients
template <typename DirectTile, typename Tile>
void create_ii_tile(TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> *C,
                    DirectTile const eri3_iii, Tile M_tile, unsigned long ord);

// Function to create the iij and jij tiles of the coefficients
template <typename DirectTile, typename Tile>
void create_ij_tile(TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> *C,
                    DirectTile direct_eri3_iij, DirectTile direct_eri3_jij,
                    Tile M_tile_ii, Tile M_tile_ij, Tile M_tile_ji,
                    Tile M_tile_jj, unsigned long ord_iij,
                    unsigned long ord_jij);

// Function to compute the by atom screener for cadf eri3 integrals
std::shared_ptr<gaussian::SchwarzScreen> cadf_by_atom_screener(
    madness::World &world, gaussian::Basis const &obs,
    gaussian::Basis const &dfbs, double threshold);

}  // namespace detail

/// Function to compute CADF fitting coefficients
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> cadf_fitting_coefficients(
    madness::World &world, gaussian::Basis const &by_cluster_obs,
    gaussian::Basis const &by_cluster_dfbs) {
  auto by_atom_cadf =
      detail::cadf_by_atom_coeffs(world, by_cluster_obs, by_cluster_dfbs);

  return detail::reblock_atom_to_clusters(by_atom_cadf, by_cluster_obs,
                                          by_cluster_dfbs);
}

// Specialization for Dense Policy although this shouldn't be used
template <>
inline TA::DistArray<TA::Tensor<double>, TA::DensePolicy>
cadf_fitting_coefficients<TA::Tensor<double>, TA::DensePolicy>(
    madness::World &world, gaussian::Basis const &obs,
    gaussian::Basis const &dfbs) {
  auto sparse_array =
      cadf_fitting_coefficients<TA::Tensor<double>, TA::SparsePolicy>(
          world, obs, dfbs);
  return TA::to_dense(sparse_array);
}

/// Function to compute seCADF 4center abab correction
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> secadf_by_atom_correction(
    madness::World &world, gaussian::Basis const &by_cluster_obs,
    gaussian::Basis const &by_cluster_dfbs, bool aaab = false) {
  ExEnv::out0() << indent << "Computing seCadf Correction" << std::endl;
  auto obs = detail::by_center_basis(by_cluster_obs);
  auto dfbs = detail::by_center_basis(by_cluster_dfbs);

  // Get direct four center integrals
  auto abab0 = mpqc::fenced_now(world);
  auto abab_shape = [&](TA::Tensor<float> const &in) {
    auto &range = in.range();
    auto ext = range.extent_data();

    using Idx_t = std::array<int, 4>;
    std::vector<std::pair<Idx_t, float>> out;
    out.reserve(ext[0] * ext[1]);

    auto fmax = std::numeric_limits<float>::max();
    for (auto a = 0; a < ext[0]; ++a) {
      out.emplace_back(std::make_pair(Idx_t{a, a, a, a}, in(a, a, a, a)));
      // out.emplace_back(std::make_pair(Idx_t{a, a, a, a}, fmax));
      for (auto b = 0; b < ext[1]; ++b) {
        if (aaab) {
          out.emplace_back(std::make_pair(Idx_t{a, a, a, b}, in(a, a, a, b)));
          out.emplace_back(std::make_pair(Idx_t{a, a, b, a}, in(a, a, b, a)));
          out.emplace_back(std::make_pair(Idx_t{a, b, a, a}, in(a, b, a, a)));
          out.emplace_back(std::make_pair(Idx_t{b, a, a, a}, in(b, a, a, a)));
          // out.emplace_back(std::make_pair(Idx_t{a, a, a, b}, fmax));
          // out.emplace_back(std::make_pair(Idx_t{a, a, b, a}, fmax));
          // out.emplace_back(std::make_pair(Idx_t{a, b, a, a}, fmax));
          // out.emplace_back(std::make_pair(Idx_t{b, a, a, a}, fmax));
        }
        out.emplace_back(std::make_pair(Idx_t{a, b, a, b}, in(a, b, a, b)));
        out.emplace_back(std::make_pair(Idx_t{a, b, b, a}, in(a, b, b, a)));
        out.emplace_back(std::make_pair(Idx_t{b, a, a, b}, in(b, a, a, b)));
        out.emplace_back(std::make_pair(Idx_t{b, a, b, a}, in(b, a, b, a)));
        // out.emplace_back(std::make_pair(Idx_t{a, b, a, b}, fmax));
        // out.emplace_back(std::make_pair(Idx_t{a, b, b, a}, fmax));
        // out.emplace_back(std::make_pair(Idx_t{b, a, a, b}, fmax));
        // out.emplace_back(std::make_pair(Idx_t{b, a, b, a}, fmax));
      }
    }

    return out;
  };

  // Commputing 4 center abab integrals
  auto eng_screen0 = mpqc::fenced_now(world);
  const auto four_array = gaussian::BasisVector{{obs, obs, obs, obs}};
  auto eng4 = make_engine_pool(libint2::Operator::coulomb,
                               utility::make_array_of_refs(obs, obs, obs, obs),
                               libint2::BraKet::xx_xx);

  auto screener = std::make_shared<gaussian::SchwarzScreen>(
      gaussian::create_schwarz_screener(world, eng4, four_array, 1e-12));
  auto eng_screen1 = mpqc::fenced_now(world);

  auto trange4 = gaussian::detail::create_trange(four_array);

  auto pmap4 =
      TA::SparsePolicy::default_pmap(world, trange4.tiles_range().volume());

  auto est0 = mpqc::fenced_now(world);
  auto abab_est = screener->norm_estimate(world, four_array, *pmap4, true);
  auto est1 = mpqc::fenced_now(world);
  auto abab_norms = abab_shape(abab_est);
  auto special1 = mpqc::fenced_now(world);

  auto ints0 = mpqc::fenced_now(world);
  auto eri4 = direct_sparse_integrals(world, eng4, four_array, abab_norms,
                                      std::move(screener));
  // auto eri4 = direct_sparse_integrals(world, eng4, four_array,
  //                                       std::move(screener));
  auto abab1 = mpqc::fenced_now(world);
  auto time_est = mpqc::duration_in_s(est0, est1);
  auto time_apply = mpqc::duration_in_s(est1, special1);
  auto time_eng = mpqc::duration_in_s(eng_screen0, eng_screen1);
  auto time_ints = mpqc::duration_in_s(ints0, abab1);
  auto time_abab = mpqc::duration_in_s(abab0, abab1);
  ExEnv::out0() << indent << "Time for all: " << time_abab << "\n";
  ExEnv::out0() << indent << "\tTime for engine and screener: " << time_eng
                << "\n";
  ExEnv::out0() << indent << "\tTime for norm est: " << time_est << "\n";
  ExEnv::out0() << indent << "\tTime for abab shape: " << time_apply << "\n";
  ExEnv::out0() << indent << "\tTime for integrals: " << time_ints << "\n";

  // detail::print_shape(eri4.array().shape().data(), "four_center_shape.csv");

  //
  // Compute C, M and 3 center integrals for robust fitting
  //
  auto eng2 = make_engine_pool(libint2::Operator::coulomb,
                               utility::make_array_of_refs(dfbs, dfbs),
                               libint2::BraKet::xs_xs);
  auto M = gaussian::sparse_integrals(world, eng2,
                                      std::vector<gaussian::Basis>{dfbs, dfbs});

  auto C = detail::cadf_by_atom_coeffs(world, by_cluster_obs, by_cluster_dfbs);
  // detail::print_shape(C.shape().data(), "C_shape.csv");

  auto eri3_screener = detail::cadf_by_atom_screener(world, obs, dfbs, 1e-12);
  auto eng3 = make_engine_pool(libint2::Operator::coulomb,
                               utility::make_array_of_refs(dfbs, obs, obs),
                               libint2::BraKet::xs_xx);

  auto three_array = std::vector<gaussian::Basis>{dfbs, obs, obs};
  auto trange3 = gaussian::detail::create_trange(three_array);
  auto pmap3 =
      TA::SparsePolicy::default_pmap(world, trange3.tiles_range().volume());

  auto E_sparse_norms_func = [&](TA::Tensor<float> const &in) {
    auto &range = in.range();
    auto ext = range.extent_data();

    using Idx_t = std::array<int, 3>;
    std::vector<std::pair<Idx_t, float>> out;

    auto fmax = std::numeric_limits<float>::max();
    for (auto a = 0; a < ext[1]; ++a) {
      // out.emplace_back(std::make_pair(Idx_t{a, a, a}, in(a, a, a)));
      out.emplace_back(std::make_pair(Idx_t{a, a, a}, fmax));

      for (auto b = 0; b < ext[2]; ++b) {
        // out.emplace_back(std::make_pair(Idx_t{a, a, b}, in(a, a, b)));
        // out.emplace_back(std::make_pair(Idx_t{b, a, b}, in(b, a, b)));
        out.emplace_back(std::make_pair(Idx_t{a, a, b}, fmax));
        out.emplace_back(std::make_pair(Idx_t{b, a, b}, fmax));
        
      }

      if (aaab) {
        for (auto X = 0; X < ext[0]; ++X) {
          // out.emplace_back(std::make_pair(Idx_t{X, a, a}, in(X, a, a)));
          out.emplace_back(std::make_pair(Idx_t{X, a, a}, fmax));
        }
      }
    }

    return out;

  };

  auto E_sparse_norms = E_sparse_norms_func(
      eri3_screener->norm_estimate(world, three_array, *pmap3, true));

  auto E = direct_sparse_integrals(world, eng3, three_array, E_sparse_norms,
                                   std::move(eri3_screener));

  // detail::print_shape(E.array().shape().data(), "E_ab_ab_shape.csv");

  auto C_contract_shape_func = [&](TA::Tensor<float> const &Cin) {
    auto const &range = Cin.range();
    auto const ext = range.extent_data();

    TA::Tensor<float> out(range, 0.0);

    const auto fmax = std::numeric_limits<float>::max();
    for (auto a = 0; a < ext[1]; ++a) {
      // if (Cin(a, a, a) != 0) {
        out(a, a, a) = fmax;
      // }

      for (auto b = 0; b < ext[2]; ++b) {
        // Check if C has or is missing a function
        auto Cmax = std::max(Cin(a, a, b), Cin(b, a, b));
        // if (Cmax != 0.0) {
          out(a, a, b) = fmax;
          out(b, a, b) = fmax;
        // }
      }

      if (aaab) {
        for (auto X = 0; X < ext[0]; ++X) {
          out(X, a, a) = fmax;
        }
      }
    }

    return out;
  };
  auto C_contract_shape = C.shape().transform(C_contract_shape_func);

  auto const &abab_data = eri4.array().shape();
  auto inputs0 = mpqc::fenced_now(world);
  TA::DistArray<Tile, Policy> MC, EC, by_atom_correction;
  MC("X, r, s") = (M("X, Y") * C("Y, r, s")).set_shape(C_contract_shape);
  EC("p,q,r,s") = (E("X, p, q") * C("X, r, s")).set_shape(abab_data);
  EC.truncate();
  MC.truncate();
  // detail::print_shape(MC.shape().data(), "MC_shape.csv");
  auto inputs1 = mpqc::fenced_now(world);
  auto time_inputs = mpqc::duration_in_s(inputs0, inputs1);
  ExEnv::out0() << indent << "Time for correction inputs: " << time_inputs
                << "\n";

  auto t2 = mpqc::fenced_now(world);
  by_atom_correction("p,q,r,s") =
      -EC("p,q,r,s") - EC("r,s,p,q") +
      (C("X, p, q") * MC("X, r, s")).set_shape(abab_data);
  auto t3 = mpqc::fenced_now(world);
  auto time_approx = mpqc::duration_in_s(t2, t3);
  ExEnv::out0() << indent << "Time for abab approx: " << time_approx << "\n";

  auto t4 = mpqc::fenced_now(world);
  by_atom_correction("p,q,r,s") += eri4("p,q,r,s");
  by_atom_correction.truncate();
  auto t5 = mpqc::fenced_now(world);
  auto time_direct_ints = mpqc::duration_in_s(t4, t5);
  ExEnv::out0() << indent << "Time for exact ints: " << time_direct_ints
                << "\n";

  double norm = by_atom_correction("p,q,r,s").norm();
  ExEnv::out0()
      << indent << "Correction Norm: " << norm << ", element average: "
      << norm / double(by_atom_correction.trange().elements_range().volume())
      << "\n";


  return by_atom_correction;
  // return TA::DistArray<TA::Tensor<double>, TA::SparsePolicy>();
}

template <>
inline TA::DistArray<TA::Tensor<double>, TA::DensePolicy>
secadf_by_atom_correction<TA::Tensor<double>, TA::DensePolicy>(
    madness::World &world, gaussian::Basis const &obs,
    gaussian::Basis const &dfbs, bool aaab_) {
  // Don't do this
  return TA::DistArray<TA::Tensor<double>, TA::DensePolicy>();
}

namespace detail {
template <typename Array, typename DirectArray>
TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> cadf_by_atom_array(
    Array const &M, DirectArray const &eri3, TA::TiledRange const &trange) {
  auto &world = M.world();
  auto Cshape = eri3.array().shape();  // cadf_shape(world, trange);

  // Use same pmap to ensure some locality
  auto pmap = eri3.array().pmap();
  TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> C(
      world, trange, std::move(Cshape), pmap);

  const auto natoms = trange.tiles_range().extent_data()[0];
  auto const &eri3_tiles = trange.tiles_range();
  auto const &M_tiles = M.trange().tiles_range();

  using ArrayType =
      typename std::remove_reference<decltype(eri3.array())>::type;
  using DirectTile = typename ArrayType::value_type;
  using Tile = TA::Tensor<double>;

  for (auto i = 0ul; i < natoms; ++i) {
    std::array<unsigned long, 3> idx_iii = {{i, i, i}};
    std::array<unsigned long, 2> idx_ii = {{i, i}};

    if (!C.is_zero(idx_iii) && C.is_local(idx_iii)) {
      auto eri3_iii = eri3.array().find(idx_iii);
      auto M_ii = M.find(idx_ii);

      unsigned long ord_iii = eri3_tiles.ordinal(idx_iii);
      world.taskq.add(create_ii_tile<DirectTile, Tile>, &C, eri3_iii, M_ii,
                      ord_iii);
    }

    for (auto j = 0ul; j < natoms; ++j) {
      if (j == i) {
        continue;
      }

      std::array<unsigned long, 3> idx_iij = {{i, i, j}};
      std::array<unsigned long, 3> idx_jij = {{j, i, j}};

      if (!C.is_zero(idx_iij) && !C.is_zero(idx_jij)) {
        if (C.is_local(idx_iij) || C.is_local(idx_jij)) {
          auto ord_iij = eri3_tiles.ordinal(idx_iij);
          auto ord_jij = eri3_tiles.ordinal(idx_jij);

          std::array<unsigned long, 2> M_idx_ii = {{i, i}};
          std::array<unsigned long, 2> M_idx_ij = {{i, j}};
          std::array<unsigned long, 2> M_idx_ji = {{j, i}};
          std::array<unsigned long, 2> M_idx_jj = {{j, j}};

          auto eri3_iij = eri3.array().find(idx_iij);
          auto eri3_jij = eri3.array().find(idx_jij);

          auto M_ii = M.find(M_idx_ii);
          auto M_ij = M.find(M_idx_ij);
          auto M_ji = M.find(M_idx_ji);
          auto M_jj = M.find(M_idx_jj);

          world.taskq.add(create_ij_tile<DirectTile, Tile>, &C, eri3_iij,
                          eri3_jij, M_ii, M_ij, M_ji, M_jj, ord_iij, ord_jij);
        }
      }
    }
  }
  world.gop.fence();
  C.truncate();

  return C;
}

template <typename DirectTile, typename Tile>
void create_ii_tile(TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> *C,
                    DirectTile eri3_iii, Tile M_tile, unsigned long ord) {
  TA::Tensor<double> eri3_tile = eri3_iii;
  auto eri3_extent = eri3_tile.range().extent();
  RowMatrixXd eri3_eig =
      TA::eigen_map(eri3_tile, eri3_extent[0], eri3_extent[1] * eri3_extent[2]);

  auto M_extent = M_tile.range().extent();
  RowMatrixXd M_eig = TA::eigen_map(M_tile, M_extent[0], M_extent[1]);

  RowMatrixXd out_eig = M_eig.inverse() * eri3_eig;

  TA::TensorD out_tile(eri3_tile.range(), 0.0);

  TA::eigen_map(out_tile, eri3_extent[0], eri3_extent[1] * eri3_extent[2]) =
      out_eig;

  C->set(ord, out_tile);
}

template <typename DirectTile, typename Tile>
void create_ij_tile(TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> *C,
                    DirectTile direct_eri3_iij, DirectTile direct_eri3_jij,
                    Tile M_tile_ii, Tile M_tile_ij, Tile M_tile_ji,
                    Tile M_tile_jj, unsigned long ord_iij,
                    unsigned long ord_jij) {
  TA::Tensor<double> eri3_iij = direct_eri3_iij;
  TA::Tensor<double> eri3_jij = direct_eri3_jij;

  auto eri3_extent_iij = eri3_iij.range().extent_data();
  auto eri3_extent_jij = eri3_jij.range().extent_data();

  RowMatrixXd eri3_eig_iij = TA::eigen_map(
      eri3_iij, eri3_extent_iij[0], eri3_extent_iij[1] * eri3_extent_iij[2]);

  RowMatrixXd eri3_eig_jij = TA::eigen_map(
      eri3_jij, eri3_extent_jij[0], eri3_extent_jij[1] * eri3_extent_jij[2]);

  auto M_extentii = M_tile_ii.range().extent_data();
  auto M_extentij = M_tile_ij.range().extent_data();
  auto M_extentji = M_tile_ji.range().extent_data();
  auto M_extentjj = M_tile_jj.range().extent_data();

  auto rows = M_extentii[0] + M_extentji[0];
  auto cols = M_extentii[1] + M_extentij[1];

  RowMatrixXd M_combo(rows, cols);

  // Write Mii
  M_combo.block(0, 0, M_extentii[0], M_extentii[1]) =
      TA::eigen_map(M_tile_ii, M_extentii[0], M_extentii[1]);

  // Write Mij
  M_combo.block(0, M_extentii[1], M_extentij[0], M_extentij[1]) =
      TA::eigen_map(M_tile_ij, M_extentij[0], M_extentij[1]);

  // Write Mji
  M_combo.block(M_extentii[0], 0, M_extentji[0], M_extentji[1]) =
      TA::eigen_map(M_tile_ji, M_extentji[0], M_extentji[1]);

  // Write Mjj
  M_combo.block(M_extentii[0], M_extentii[1], M_extentjj[0], M_extentjj[1]) =
      TA::eigen_map(M_tile_jj, M_extentjj[0], M_extentjj[1]);

  RowMatrixXd M_combo_inv = M_combo.inverse();

  // Doing the block wise GEMM by hand for now.
  auto blockii = M_combo_inv.block(0, 0, M_extentii[0], M_extentii[1]);

  auto blockij =
      M_combo_inv.block(0, M_extentii[1], M_extentij[0], M_extentij[1]);

  auto blockji =
      M_combo_inv.block(M_extentii[0], 0, M_extentji[0], M_extentji[1]);

  auto blockjj = M_combo_inv.block(M_extentii[0], M_extentii[1], M_extentjj[0],
                                   M_extentjj[1]);

  if (C->is_local(ord_iij)) {
    RowMatrixXd C_iij = blockii * eri3_eig_iij + blockij * eri3_eig_jij;

    TA::Tensor<double> out_iij(eri3_iij.range());
    TA::eigen_map(out_iij, eri3_extent_iij[0],
                  eri3_extent_iij[1] * eri3_extent_iij[2]) = C_iij;

    C->set(ord_iij, out_iij);
  }
  if (C->is_local(ord_jij)) {
    RowMatrixXd C_jij = blockji * eri3_eig_iij + blockjj * eri3_eig_jij;

    TA::Tensor<double> out_jij(eri3_jij.range());
    TA::eigen_map(out_jij, eri3_extent_jij[0],
                  eri3_extent_jij[1] * eri3_extent_jij[2]) = C_jij;

    C->set(ord_jij, out_jij);
  }
}

template <typename Array>
TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> reblock_atom_to_clusters(
    Array const &C_atom, gaussian::Basis const &obs,
    gaussian::Basis const &dfbs) {
  auto trange = gaussian::detail::create_trange(
      std::array<gaussian::Basis, 3>{dfbs, obs, obs});

  std::unordered_map<int64_t, std::vector<int64_t>> c2a;
  auto shape = cadf_shape_cluster(C_atom, trange, c2a);

  auto &world = C_atom.world();

  TA::DistArray<TA::Tensor<double>, TA::SparsePolicy> C(world, trange, shape);
  C.fill(0.0);

  auto write_inner_tile = [&](TA::Tensor<double> const &inner,
                              TA::Tensor<double> &out) {
    auto in_start = inner.range().lobound();
    auto in_end = inner.range().upbound();

    out.block(in_start, in_end) = inner;
  };

  const auto volume = trange.tiles_range().volume();
  for (auto ord = 0ul; ord < volume; ++ord) {
    if (!C.is_zero(ord) && C.is_local(ord)) {
      auto &tile = C.find(ord).get();

      for (auto const &atom_ord : c2a.find(int64_t(ord))->second) {
        write_inner_tile(C_atom.find(atom_ord), tile);
      }
    }
  }
  world.gop.fence();
  C.truncate();

  return C;
}

}  // namespace detail

}  // namespace lcao
}  // namespace mpqc
#endif  //  MPQC_CHEMISTRY_QC_LCAO_INTEGRALS_DENSITYFITTING_CADF_COEFFS_H
