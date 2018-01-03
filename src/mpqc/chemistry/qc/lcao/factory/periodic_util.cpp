#include "mpqc/chemistry/qc/lcao/factory/periodic_util.h"

namespace mpqc {
namespace lcao {

namespace detail {

TA::TiledRange1 extend_trange1(TA::TiledRange1 const &tr0, int64_t size) {
  auto blocking = std::vector<int64_t>{0};
  for (auto idx = 0; idx < size; ++idx) {
    for (auto u = 0; u < tr0.tile_extent(); ++u) {
      auto next = blocking.back() + tr0.tile(u).second - tr0.tile(u).first;
      blocking.emplace_back(next);
    }
  }
  TA::TiledRange1 tr1(blocking.begin(), blocking.end());
  return tr1;
}

void sort_eigen(VectorZ &eigVal, MatrixZ &eigVec) {
  auto val = eigVal.real();

  // Sort by ascending eigenvalues
  std::vector<std::pair<double, int>> sortedVal;
  sortedVal.reserve(val.size());
  for (auto i = 0; i != val.size(); ++i) {
    auto pair = std::make_pair(val(i), i);
    sortedVal.push_back(pair);
  };
  std::sort(sortedVal.begin(), sortedVal.end());

  // Build sorted eigenvalues and eigenvectors
  VectorZ sortedEigVal(eigVal);
  MatrixZ sortedEigVec(eigVec);
  for (auto i = 0; i != val.size(); ++i) {
    sortedEigVal(i) = eigVal(sortedVal[i].second);
    sortedEigVec.col(i) = eigVec.col(sortedVal[i].second);
  }

  eigVal = sortedEigVal;
  eigVec = sortedEigVec;
}

Vector3d direct_vector(int64_t ord_idx, Vector3i const &lattice_max,
                       Vector3d const &dcell) {
  auto z = ord_idx % (2 * lattice_max(2) + 1);
  auto y = (ord_idx / (2 * lattice_max(2) + 1)) % (2 * lattice_max(1) + 1);
  auto x = ord_idx / (2 * lattice_max(2) + 1) / (2 * lattice_max(1) + 1);
  Vector3d result((x - lattice_max(0)) * dcell(0),
                  (y - lattice_max(1)) * dcell(1),
                  (z - lattice_max(2)) * dcell(2));
  return result;
}

Vector3d k_vector(int64_t ord_idx, Vector3i const &nk, Vector3d const &dcell) {
  Vector3d result;
  auto x = ord_idx / nk(2) / nk(1);
  auto y = (ord_idx / nk(2)) % nk(1);
  auto z = ord_idx % nk(2);
  result(0) = (dcell(0) == 0.0)
                  ? 0.0
                  : (-1.0 + (2.0 * (x + 1) - 1.0) / nk(0)) * (M_PI / dcell(0));
  result(1) = (dcell(1) == 0.0)
                  ? 0.0
                  : (-1.0 + (2.0 * (y + 1) - 1.0) / nk(1)) * (M_PI / dcell(1));
  result(2) = (dcell(2) == 0.0)
                  ? 0.0
                  : (-1.0 + (2.0 * (z + 1) - 1.0) / nk(2)) * (M_PI / dcell(2));
  return result;
}

int64_t direct_ord_idx(Vector3i const &in_3D_idx, Vector3i const &lattice_max) {
  return direct_ord_idx(in_3D_idx(0), in_3D_idx(1), in_3D_idx(2), lattice_max);
}

int64_t direct_ord_idx(int64_t x, int64_t y, int64_t z,
                       Vector3i const &lattice_max) {
  if (lattice_max(0) >= 0 && lattice_max(1) >= 0 && lattice_max(2) >= 0 &&
      std::abs(x) <= lattice_max(0) && std::abs(y) <= lattice_max(1) &&
      std::abs(z) <= lattice_max(2)) {
    int64_t idx = (x + lattice_max(0)) * (2 * lattice_max(1) + 1) *
                      (2 * lattice_max(2) + 1) +
                  (y + lattice_max(1)) * (2 * lattice_max(2) + 1) +
                  (z + lattice_max(2));
    return idx;
  } else {
    throw "invalid lattice sum index/boundaries";
  }
}

Vector3i direct_3D_idx(const int64_t ord_idx, Vector3i const &lattice_max) {
  if (lattice_max(0) >= 0 && lattice_max(1) >= 0 && lattice_max(2) >= 0) {
    auto z = ord_idx % (2 * lattice_max(2) + 1);
    auto y = (ord_idx / (2 * lattice_max(2) + 1)) % (2 * lattice_max(1) + 1);
    auto x = ord_idx / (2 * lattice_max(2) + 1) / (2 * lattice_max(1) + 1);

    Vector3i result(x - lattice_max(0), y - lattice_max(1), z - lattice_max(2));
    return result;
  } else {
    throw "invalid lattice boundaries";
  }
}

int64_t k_ord_idx(Vector3i const &in_3D_idx, Vector3i const &nk) {
  return k_ord_idx(in_3D_idx(0), in_3D_idx(1), in_3D_idx(2), nk);
}

int64_t k_ord_idx(int64_t x, int64_t y, int64_t z, Vector3i const &nk) {
  if (nk(0) >= 1 && nk(1) >= 1 && nk(2) >= 1 && x >= 0 && y >= 0 && z >= 0 &&
      x < nk(0) && y < nk(1) && z < nk(2)) {
    int64_t idx = x * nk(1) * nk(2) + y * nk(2) + z;
    return idx;
  } else {
    throw "invalid k-space index/boundaries";
  }
}

std::shared_ptr<Molecule> shift_mol_origin(Molecule const &mol,
                                           Vector3d const &shift) {
  std::vector<AtomBasedClusterable> vec_of_clusters;
  for (auto &cluster : mol) {
    AtomBasedCluster shifted_cluster;
    for (auto &atom : collapse_to_atoms(cluster)) {
      Atom shifted_atom(atom.center() + shift, atom.mass(), atom.charge());
      shifted_cluster.add_clusterable(shifted_atom);
    }
    shifted_cluster.update_cluster();
    vec_of_clusters.emplace_back(shifted_cluster);
  }

  Molecule result(vec_of_clusters);

  auto result_ptr = std::make_shared<Molecule>(result);
  return result_ptr;
}

}  // namespace detail

namespace gaussian {
namespace detail {

std::shared_ptr<Basis> shift_basis_origin(const Basis &basis,
                                          const Vector3d &shift) {
  std::vector<ShellVec> vec_of_shells;
  for (auto shell_vec : basis.cluster_shells()) {
    ShellVec shells;
    for (auto shell : shell_vec) {
      std::array<double, 3> new_origin = {{shell.O[0] + shift(0),
                                           shell.O[1] + shift(1),
                                           shell.O[2] + shift(2)}};
      shell.move(new_origin);
      shells.push_back(shell);
    }
    vec_of_shells.push_back(shells);
  }
  Basis result(vec_of_shells);
  auto result_ptr = std::make_shared<Basis>(result);
  return result_ptr;
}

std::shared_ptr<Basis> shift_basis_origin(const Basis &basis,
                                          const Vector3d &shift_base,
                                          const Vector3i &nshift,
                                          const Vector3d &dcell,
                                          const bool is_half_range) {
  std::vector<ShellVec> vec_of_shells;

  using ::mpqc::lcao::detail::direct_ord_idx;
  using ::mpqc::lcao::detail::direct_vector;
  int64_t shift_size = 1 + direct_ord_idx(nshift, nshift);
  assert(shift_size > 0 && shift_size % 2 == 1);
  int64_t start = is_half_range ? ((shift_size - 1) / 2) : 0;

  for (auto idx_shift = start; idx_shift < shift_size; ++idx_shift) {
    Vector3d shift = direct_vector(idx_shift, nshift, dcell) + shift_base;

    for (auto shell_vec : basis.cluster_shells()) {
      ShellVec shells;
      for (auto shell : shell_vec) {
        std::array<double, 3> new_origin = {{shell.O[0] + shift(0),
                                             shell.O[1] + shift(1),
                                             shell.O[2] + shift(2)}};
        shell.move(new_origin);
        shells.push_back(shell);
      }
      vec_of_shells.push_back(shells);
    }
  }

  Basis result(vec_of_shells);
  auto result_ptr = std::make_shared<Basis>(result);
  return result_ptr;
}

}  // namespace detail
}  // namespace gaussian
}  // namespace lcao
}  // namespace mpqc
