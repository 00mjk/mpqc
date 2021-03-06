#ifndef SRC_MPQC_CHEMISTRY_QC_LCAO_CC_SOLVERS_H_
#define SRC_MPQC_CHEMISTRY_QC_LCAO_CC_SOLVERS_H_


#include "mpqc/chemistry/molecule/common.h"
#include "mpqc/chemistry/qc/cc/solvers.h"
#include "mpqc/chemistry/qc/lcao/factory/factory.h"
#include "mpqc/math/linalg/diagonal_array.h"
#include "mpqc/math/tensor/clr/array_to_eigen.h"
#include "../../../../util/core/exenv.h"
#include "../../cc/solvers.h"

namespace mpqc {
namespace lcao {
namespace cc {

enum class PNORankUpdateMethod {
  standard, fuzzy, fixed
};

namespace detail {
inline void print_local(TiledArray::World& world,
                        const std::shared_ptr<TiledArray::Pmap>& pmap) {
  for (ProcessID r = 0; r < world.size(); ++r) {
    world.gop.fence();
    if (r == world.rank()) {
      std::cout << r << ": { ";
      for (TiledArray::Pmap::const_iterator it = pmap->begin();
           it != pmap->end(); ++it)
        std::cout << *it << " ";
      std::cout << "}\n";
    }
  }
}

// squared norm of 1-body residual in OSV subspace
template <typename T>
struct R1SquaredNormReductionOp {
  // typedefs
  typedef typename TA::detail::scalar_type<T>::type result_type;
  typedef typename T::value_type argument_type;
  using Matrix = RowMatrix<result_type>;

  R1SquaredNormReductionOp(const std::vector<Matrix>& r1_space)
      : r1_space_(r1_space) {}

  // Reduction functions
  // Make an empty result object
  result_type operator()() const { return 0; }
  // Post process the result (no operation, passthrough)
  const result_type& operator()(const result_type& result) const {
    return result;
  }
  void operator()(result_type& result, const result_type& arg) const {
    result += arg;
  }
  /// Reduce an argument pair
  void operator()(result_type& result, const argument_type& arg) const {
    const auto i = arg.range().lobound()[1];
    const auto nuocc = arg.range().extent_data()[0];

    const Matrix arg_osv = TA::eigen_map(arg, 1, nuocc) * r1_space_[i];
    result += arg_osv.squaredNorm();
  }

  const std::vector<Matrix>& r1_space_;
};  // R1SquaredNormReductionOp

// squared norm of 2-body residual in PNO subspace
template <typename T>
struct R2SquaredNormReductionOp {
  // typedefs
  typedef typename TA::detail::scalar_type<T>::type result_type;
  typedef typename T::value_type argument_type;
  using Matrix = RowMatrix<result_type>;

  R2SquaredNormReductionOp(const std::vector<Matrix>& r2_space)
      : r2_space_(r2_space), nocc_act_(std::sqrt(r2_space.size())) {}

  // Reduction functions
  // Make an empty result object
  result_type operator()() const { return 0; }
  // Post process the result (no operation, passthrough)
  const result_type& operator()(const result_type& result) const {
    return result;
  }
  void operator()(result_type& result, const result_type& arg) const {
    result += arg;
  }
  /// Reduce an argument pair
  void operator()(result_type& result, const argument_type& arg) const {
    const auto i = arg.range().lobound()[2];
    const auto j = arg.range().lobound()[3];
    const auto nuocc = arg.range().extent_data()[0];

    const auto r2_index = i * nocc_act_ + j;
    const Matrix arg_pno = r2_space_[r2_index].transpose() *
        TA::eigen_map(arg, nuocc, nuocc) *
        r2_space_[r2_index];
    result += arg_pno.squaredNorm();
  }

  const std::vector<Matrix>& r2_space_;
  std::size_t nocc_act_;
};  // R2SquaredNormReductionOp

template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> jacobi_update_t3_abcijk(
    const TA::DistArray<Tile, Policy>& r3_abcijk,
    const EigenVector<typename Tile::numeric_type>& ens_occ,
    const EigenVector<typename Tile::numeric_type>& ens_uocc) {
    auto denom = [ens_occ, ens_uocc](Tile& result_tile, const Tile& arg_tile) {

    result_tile = Tile(arg_tile.range());

    // compute index
    const auto a0 = result_tile.range().lobound()[0];
    const auto an = result_tile.range().upbound()[0];
    const auto b0 = result_tile.range().lobound()[1];
    const auto bn = result_tile.range().upbound()[1];
    const auto c0 = result_tile.range().lobound()[2];
    const auto cn = result_tile.range().upbound()[2];
    const auto i0 = result_tile.range().lobound()[3];
    const auto in = result_tile.range().upbound()[3];
    const auto j0 = result_tile.range().lobound()[4];
    const auto jn = result_tile.range().upbound()[4];
    const auto k0 = result_tile.range().lobound()[5];
    const auto kn = result_tile.range().upbound()[5];

    auto tile_idx = 0;
    typename Tile::scalar_type norm = 0.0;
    for (auto a = a0; a < an; ++a) {
      const auto e_a = ens_uocc[a];
      for (auto b = b0; b < bn; ++b) {
        const auto e_b = ens_uocc[b];
        for (auto c = c0; c < cn; ++c) {
          const auto e_c = ens_uocc[c];
        for (auto i = i0; i < in; ++i) {
          const auto e_i = ens_occ[i];
          for (auto j = j0; j < jn; ++j) {
            const auto e_j = ens_occ[j];
            for (auto k = k0; k < kn; ++k, ++tile_idx) {
              const auto e_k = ens_occ[k];
            const auto e_iajbkc = e_i + e_j + e_k - e_a - e_b - e_c ;
            const auto old = arg_tile[tile_idx];
            const auto result_abcijk = old / (e_iajbkc);
            const auto abs_result_abcijk = std::abs(result_abcijk);
            norm += abs_result_abcijk * abs_result_abcijk;
            result_tile[tile_idx] = result_abcijk;
          }
        }
       }
      }
     }
    }
    return std::sqrt(norm);
  };

  auto delta_t3_abcijk = TA::foreach (r3_abcijk, denom);
  delta_t3_abcijk.world().gop.fence();
  return delta_t3_abcijk;
}


template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> jacobi_update_t2_abij(
    const TA::DistArray<Tile, Policy>& r2_abij,
    const EigenVector<typename Tile::numeric_type>& ens_occ,
    const EigenVector<typename Tile::numeric_type>& ens_uocc) {
  auto denom = [&ens_occ, &ens_uocc](Tile& result_tile, const Tile& arg_tile) {

    result_tile = Tile(arg_tile.range());

    // compute index
    const auto a0 = result_tile.range().lobound()[0];
    const auto an = result_tile.range().upbound()[0];
    const auto b0 = result_tile.range().lobound()[1];
    const auto bn = result_tile.range().upbound()[1];
    const auto i0 = result_tile.range().lobound()[2];
    const auto in = result_tile.range().upbound()[2];
    const auto j0 = result_tile.range().lobound()[3];
    const auto jn = result_tile.range().upbound()[3];

    auto tile_idx = 0;
    typename Tile::scalar_type norm = 0.0;
    for (auto a = a0; a < an; ++a) {
      const auto e_a = ens_uocc[a];
      for (auto b = b0; b < bn; ++b) {
        const auto e_b = ens_uocc[b];
        for (auto i = i0; i < in; ++i) {
          const auto e_i = ens_occ[i];
          for (auto j = j0; j < jn; ++j, ++tile_idx) {
            const auto e_j = ens_occ[j];
            const auto e_iajb = e_i + e_j - e_a - e_b;
            const auto old = arg_tile[tile_idx];
            const auto result_abij = old / (e_iajb);
            const auto abs_result_abij = std::abs(result_abij);
            norm += abs_result_abij * abs_result_abij;
            result_tile[tile_idx] = result_abij;
          }
        }
      }
    }
    return std::sqrt(norm);
  };

  auto delta_t2_abij = TA::foreach (r2_abij, denom);
  delta_t2_abij.world().gop.fence();
  return delta_t2_abij;
}

/**
 *
 * @tparam Tile
 * @tparam Policy
 * @param[in] r1_ai T1-like array with dimension "a,i"
 * @param[in] ens_occ 1-dimensional array consisting of the occ-occ diagonal
 * elements of the Fock matrix or the occupied orbital epsilon values
 * @param[in] ens_uocc 1-dimensional array consisting of the unocc-unocc
 * diagonal elements of the Fock matrix or the unoccupied orbital epsilong values
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> jacobi_update_t1_ai(
    const TA::DistArray<Tile, Policy>& r1_ai,
    const EigenVector<typename Tile::numeric_type>& ens_occ,
    const EigenVector<typename Tile::numeric_type>& ens_uocc) {
  auto denom = [&ens_occ, &ens_uocc](Tile& result_tile, const Tile& arg_tile) {

    result_tile = Tile(arg_tile.range());

    // compute index
    const auto a0 = result_tile.range().lobound()[0];
    const auto an = result_tile.range().upbound()[0];
    const auto i0 = result_tile.range().lobound()[1];
    const auto in = result_tile.range().upbound()[1];

    auto tile_idx = 0;
    typename Tile::scalar_type norm = 0.0;
    for (auto a = a0; a < an; ++a) {
      const auto e_a = ens_uocc[a];
      for (auto i = i0; i < in; ++i, ++tile_idx) {
        const auto e_i = ens_occ[i];
        const auto e_ia = e_i - e_a;
        const auto old = arg_tile[tile_idx];
        const auto result_ai = old / (e_ia);
        const auto abs_result_ai = std::abs(result_ai);
        norm += abs_result_ai * abs_result_ai;
        result_tile[tile_idx] = result_ai;
      }
    }
    return std::sqrt(norm);
  };

  auto delta_t1_ai = TA::foreach (r1_ai, denom);
  delta_t1_ai.world().gop.fence();
  return delta_t1_ai;
}

/**
 *
 * @param[in] r2_abij  T2 like Array with dimension "a,b,i,j", it should blocked by
 * V in a,b and 1 in i,j
 * @param[in] eps_occ_act  the diagonal of active occupied Fock matrix in canonical basis
 * @param[in] eps_pno vector of diagonal Fock matrix in PNO basis, only local i,j
 * will be initialized
 * @param[in] pnos vector of PNOs, only local i,j will be initialized
 * @param[in] shift value of the shift in the denominator (i.e. the demonimator is @c F[i]+F[j]-F[a]-F[b]+shift
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> pno_jacobi_update_t2(
    const TA::DistArray<Tile, Policy>& r2_abij,
    const EigenVector<typename Tile::numeric_type>& eps_occ_act,
    const std::vector<EigenVector<typename Tile::numeric_type>>& eps_pno,
    const std::vector<RowMatrix<typename Tile::numeric_type>>& pnos,
    typename Tile::numeric_type shift = 0.0) {
  auto update2 = [eps_occ_act, eps_pno, pnos, shift](Tile& result_tile,
                                                      const Tile& arg_tile) {

    result_tile = Tile(arg_tile.range());

    // determine i and j indices
    const auto i = arg_tile.range().lobound()[2];
    const auto j = arg_tile.range().lobound()[3];

    // Select appropriate matrix of PNOs
    auto ij = i * eps_occ_act.size() + j;
    Eigen::MatrixXd pno_ij = pnos[ij];

    // Extent data of tile
    const auto ext = arg_tile.range().extent_data();

    // Convert data in tile to Eigen::Map and transform to PNO basis
    const Eigen::MatrixXd r2_pno =
        pno_ij.transpose() *
        TA::eigen_map(arg_tile, ext[0] * ext[2], ext[1] * ext[3]) * pno_ij;

    // Create a matrix delta_t2_pno to hold updated values of delta_t2 in PNO
    // basis this matrix will then be back transformed to full basis before
    // being converted to a tile
    Eigen::MatrixXd delta_t2_pno = r2_pno;

    // Select correct vector containing diagonal elements of Fock matrix in
    // PNO basis
    const Eigen::VectorXd& ens_uocc = eps_pno[ij];

    // Determine number of PNOs
    const auto npno = ens_uocc.rows();

    // Determine number of uocc
    const auto nuocc = pno_ij.rows();

    // Select e_i and e_j
    const auto e_ij = eps_occ_act(i) + eps_occ_act(j) + shift;

    for (auto a = 0; a < npno; ++a) {
      const auto e_a = ens_uocc[a];
      for (auto b = 0; b < npno; ++b) {
        const auto e_b = ens_uocc[b];
        const auto e_abij = e_ij - e_a - e_b;
        const auto r_abij = r2_pno(a, b);
        delta_t2_pno(a, b) = r_abij / e_abij;
      }
    }

    // Back transform delta_t2_pno to full space
    Eigen::MatrixXd delta_t2_full = pno_ij * delta_t2_pno * pno_ij.transpose();

    // Convert delta_t2_full to tile and compute norm
    typename Tile::scalar_type norm = 0.0;
    for (auto r = 0; r < nuocc; ++r) {
      for (auto c = 0; c < nuocc; ++c) {
        const auto idx = r * nuocc + c;
        const auto elem = delta_t2_full(r, c);
        const auto abs_elem = std::abs(elem);
        norm += abs_elem * abs_elem;
        result_tile[idx] = elem;
      }
    }

    return std::sqrt(norm);
  };
  auto delta_t2_abij = TA::foreach (r2_abij, update2);
  delta_t2_abij.world().gop.fence();
  return delta_t2_abij;
}

/**
 *
 * @param[in] r1_ai  T1 like Array with dimension "a,i", it should blocked by V in
 * a,b and 1 in i,j
 * @param[in] eps_occ_act  the diagonal of active occupied Fock matrix in canonical basis
 * @param[in] eps_osv vector of the diagonal of Fock matrix in OSV basis, only local i,j
 * will be initialized
 * @param[in] osvs vector of OSVs, only local i will be initialized
 * @param[in] shift value of the shift in the denominator (i.e. the demonimator is @c F[i]-F[a]+shift
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> pno_jacobi_update_t1(
    const TA::DistArray<Tile, Policy>& r1_ai,
    const EigenVector<typename Tile::numeric_type>& eps_occ_act,
    const std::vector<EigenVector<typename Tile::numeric_type>>& eps_osv,
    const std::vector<RowMatrix<typename Tile::numeric_type>>& osvs,
    typename Tile::numeric_type shift = 0.0) {
  auto update1 = [eps_occ_act, eps_osv, osvs, shift](Tile& result_tile,
                                                      const Tile& arg_tile) {

    result_tile = Tile(arg_tile.range());

    // determine i index
    const auto i = arg_tile.range().lobound()[1];

    // Select appropriate matrix of OSVs
    Eigen::MatrixXd osv_i = osvs[i];

    // Extent data of tile
    const auto ext = arg_tile.range().extent_data();

    // Convert data in tile to Eigen::Map and transform to OSV basis
    const Eigen::VectorXd r1_osv =
        osv_i.transpose() * TA::eigen_map(arg_tile, ext[0], ext[1]);

    // Create a matrix delta_t1_osv to hold updated values of delta t1 in OSV
    // basis. This matrix will then be back transformed to full basis before
    // being converted to a tile
    Eigen::VectorXd delta_t1_osv = r1_osv;

    // Select correct vector containing diagonal elements of Fock matrix in
    // OSV basis
    const Eigen::VectorXd& ens_uocc = eps_osv[i];

    // Determine number of OSVs
    //      const auto nosv = ens_uocc.rows();
    const auto nosv = osv_i.cols();

    // Determine number of uocc
    const auto nuocc = osv_i.rows();

    const auto e_i = eps_occ_act(i) + shift;

    for (auto a = 0; a < nosv; ++a) {
      const auto e_a = ens_uocc[a];
      const auto e_ai = e_i - e_a;
      const auto r_ai = r1_osv(a);
      delta_t1_osv(a) = r_ai / e_ai;
    }

    // Back transform delta_t1_osv to full space
    Eigen::VectorXd delta_t1_full = osv_i * delta_t1_osv;

    // Convert delta_t1_full to tile and compute norm
    typename Tile::scalar_type norm = 0.0;
    for (auto r = 0; r < nuocc; ++r) {
      const auto elem = delta_t1_full(r);
      const auto abs_elem = std::abs(elem);
      norm += abs_elem * abs_elem;
      result_tile[r] = elem;
    }

    return std::sqrt(norm);
  };

  auto delta_t1_ai = TA::foreach (r1_ai, update1);
  delta_t1_ai.world().gop.fence();
  return delta_t1_ai;
}

/**
 *
 * @param[in] abij T2 like Array with dimension "a,b,i,j", it should blocked by V in
 * a,b and 1 in i,j
 * @param[in] pnos vector of PNOs, only local i,j will be initialized
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> pno_transform_abij(
    const TA::DistArray<Tile, Policy>& abij,
    const std::vector<RowMatrix<typename Tile::numeric_type>>& pnos) {
  std::size_t nocc_act = abij.trange().dim(2).extent();

  auto tform = [&pnos, nocc_act](Tile& result_tile, const Tile& arg_tile) {

    // determine i and j indices
    const long i = arg_tile.range().lobound()[2];
    const long j = arg_tile.range().lobound()[3];

    // Select appropriate matrix of PNOs
    const auto ij = i * nocc_act + j;
    Eigen::MatrixXd pno_ij = pnos[ij];
    const auto nuocc = pno_ij.rows();
    const auto npno = pno_ij.cols();

    // Convert data in tile to Eigen::Map and transform to PNO basis
    const Eigen::MatrixXd result_eig =
        pno_ij.transpose() * TA::eigen_map(arg_tile, nuocc, nuocc) * pno_ij;

    // Convert result_eig to tile and compute norm
    result_tile = Tile(TA::Range({std::make_pair(0l,npno),
                                  std::make_pair(0l,npno),
                                  std::make_pair(i,i+1),
                                  std::make_pair(j,j+1)}));
    typename Tile::scalar_type norm = 0.0;
    for (auto r = 0; r < npno; ++r) {
      for (auto c = 0; c < npno; ++c) {
        const auto idx = r * npno + c;
        const auto elem = result_eig(r, c);
        const auto abs_elem = std::abs(elem);
        norm += abs_elem * abs_elem;
        result_tile[idx] = elem;
      }
    }

    return std::sqrt(norm);
  };

  auto result = TA::foreach (abij, tform);
  result.world().gop.fence();
  return result;
}

/**
 *
 * @param[in] ai  T1 like Array with dimension "a,i", it should blocked by V in a,b
 * and 1 in i,j
 * @param[in] osvs vector of OSVs, only local i will be initialized
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> osv_transform_ai(
    const TA::DistArray<Tile, Policy>& ai,
    const std::vector<RowMatrix<typename Tile::numeric_type>>& osvs) {
  auto tform = [&osvs](Tile& result_tile, const Tile& arg_tile) {

    // determine i index
    const auto i = arg_tile.range().lobound()[1];

    // Select appropriate matrix of OSVs
    Eigen::MatrixXd osv_i = osvs[i];
    const auto nuocc = osv_i.rows();
    const auto nosv = osv_i.cols();

    // Convert data in tile to Eigen::Map and transform to OSV basis
    const Eigen::MatrixXd result_eig =
        osv_i.transpose() * TA::eigen_map(arg_tile, nuocc, 1);

    // Convert result_eig to tile and compute norm
    result_tile = Tile(TA::Range{nosv, 1l});
    typename Tile::scalar_type norm = 0.0;
    for (auto r = 0; r < nosv; ++r) {
      const auto elem = result_eig(r, 0);
      const auto abs_elem = std::abs(elem);
      norm += abs_elem * abs_elem;
      result_tile[r] = elem;
    }

    return std::sqrt(norm);
  };

  auto result = TA::foreach (ai, tform);
  result.world().gop.fence();
  return result;
}

// "Project" T2 into new PNO space
/**
 *
 * @tparam Tile
 * @tparam Policy
 * @param[in] t2 T2-like array with dimensions "a,b,i,j"
 * @param[in] pnos Vector of PNO matrices
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> t2_project_pno(
    const TA::DistArray<Tile, Policy>& t2,
    std::vector<RowMatrix<typename Tile::numeric_type>>& pnos) {
  using Matrix = RowMatrix<typename Tile::numeric_type>;

  std::size_t nocc_act = t2.trange().dim(2).extent();
  std::size_t nuocc = t2.trange().dim(0).extent();

  auto project_t2 = [nuocc, nocc_act, pnos](Tile& result_tile, const Tile& arg_tile) {
    result_tile = Tile(arg_tile.range());

    // Get values of i and j
    const int i = arg_tile.range().lobound()[2];
    const int j = arg_tile.range().lobound()[3];

    // Select appropriate PNO matrix
    Matrix pno_ij = pnos[i*nocc_act + j];

    // Turn tile of t2 into matrix
    Matrix t2_ij = TA::eigen_map(arg_tile, nuocc, nuocc);

    // Project t2
    Matrix t2_ij_prime = pno_ij.transpose() * t2_ij * pno_ij;

    // Project back
    Matrix T2_ij = pno_ij * t2_ij_prime * pno_ij.transpose();

    // Turn T_ij back into tile
    auto norm = 0.0;
    for (int a = 0, tile_idx = 0; a != nuocc; ++a) {
      for (int b = 0; b != nuocc; ++b, ++tile_idx) {
        const auto elem = T2_ij(a, b);
        const auto abs_result = std::abs(elem);
        norm += (abs_result * abs_result);
        result_tile[tile_idx] = elem;
      }
    }
    return std::sqrt(norm);
  }; // project_t2

  auto T2 = TA::foreach(t2, project_t2);
  T2.world().gop.fence();
  return T2;
};  // t2_project_pno

/**
 *
 * @param[in] t2   T2 like array with dimension "a,b,i,j"
 * @param[in] i_block  array with dimension "i,j" that reblocks occupied dimension i to j
 * @param[in] a_block  array with dimension "a,b" that reblocks unoccupied dimension a to b
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> reblock_t2(
    const TA::DistArray<Tile, Policy>& t2,
    const TA::DistArray<Tile, Policy>& i_block,
    const TA::DistArray<Tile, Policy>& a_block) {
  // Reblock T2
  TA::DistArray<Tile, Policy> result;
  result("an,bn,in,jn") = t2("a,b,i,j") * i_block("j,jn") * i_block("i,in") *
                          a_block("b,bn") * a_block("a,an");

  return result;
}

/**
 *
 * @param[in] t2   T2-like array with dimension "a,b,i,j"
 * @param[in] i_block  array with dimension "i,j" that reblocks occupied dimension j to i
 * @param[in] a_block  array with dimension "a,b" that reblocks unoccupied dimension b to a
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> unblock_t2(
    const TA::DistArray<Tile, Policy>& t2,
    const TA::DistArray<Tile, Policy>& i_block,
    const TA::DistArray<Tile, Policy>& a_block) {
  // Reblock T2
  TA::DistArray<Tile, Policy> result;
  result("an,bn,in,jn") = t2("a,b,i,j") * i_block("jn,j") * i_block("in,i") *
                          a_block("bn,b") * a_block("an,a");

  return result;
}

/**
 *
 * @param[in] t1  T1-like array with dimension "a,i"
 * @param[in] i_block   array with dimension "i,j" that reblocks occupied dimension i to j
 * @param[in] a_block   array with dimension "a,b" that reblocks unoccupied dimension a to b
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> reblock_t1(
    const TA::DistArray<Tile, Policy>& t1,
    const TA::DistArray<Tile, Policy>& i_block,
    const TA::DistArray<Tile, Policy>& a_block) {
  // Create target_pmap to match OSVs pma
  auto nocc_act = i_block.range().extent_data()[1];
  auto target_pmap = Policy::default_pmap(t1.world(), nocc_act);

  // Reblock T1
  TA::DistArray<Tile, Policy> result;
  result("an,in") = (t1("a,i") * i_block("i,in") * a_block("a,an")).set_pmap(target_pmap);
  return result;
}

/**
 *
 * @param[in] t1  T1-like array with dimension "a,i"
 * @param[in] i_block   array with dimension "i,j" that reblocks occupied dimension j to i
 * @param[in] a_block   array with dimension "a,b" that reblocks unoccupied dimension b to a
 * @return
 */
template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> unblock_t1(
    const TA::DistArray<Tile, Policy>& t1,
    const TA::DistArray<Tile, Policy>& i_block,
    const TA::DistArray<Tile, Policy>& a_block) {
  // Reblock T1
  TA::DistArray<Tile, Policy> result;
  result("an,in") = t1("a,i") * i_block("in,i") * a_block("an,a");
  return result;
}

/**
  *
  * @param[in] K   T2-like array wth dimension "a,b,i,j"
  * @param[in] t2  T2-like array with dimension "a,b,i,j"
  * @return
  */
template <typename Tile, typename Policy>
double compute_mp2(
    const TA::DistArray<Tile, Policy>& K,
    const TA::DistArray<Tile, Policy>& t2) {
  // Compute the MP2 correlation energy
  double e_mp2 = TA::dot(2*K("a,b,i,j") - K("b,a,i,j"), t2("a,b,i,j"));
  return e_mp2;
}

/**
 *
 * @tparam Tile
 * @tparam Policy
 * @param[in] K  T2-like array with dimension "a,b,i,j"
 * @param[in] F_occ_act  active occ-active occ block of the Fock matrix
 * @param[in] F_uocc   unocc-unocc block of the Fock matrix
 * @param[in] pnos  vector of PNO matrices
 * @param[in] nocc_act   integer specifying the number of active occupied orbitals
 * @return
 */
template<typename Tile, typename Policy>
TA::DistArray<Tile, Policy> form_T_from_K (
    const TA::DistArray<Tile, Policy>& K,
    const RowMatrix<typename Tile::numeric_type>& F_occ_act,
    const RowMatrix<typename Tile::numeric_type>& F_uocc,
    std::vector<RowMatrix<typename Tile::numeric_type>>& pnos,
    int nocc_act) {

  using Matrix = RowMatrix<typename Tile::numeric_type>;
  using Vector = EigenVector<typename Tile::numeric_type>;

  auto form_T = [&F_occ_act, &F_uocc, &pnos, nocc_act](Tile& result_tile, const Tile& arg_tile) {

    result_tile = Tile(arg_tile.range());

    // determine i and j indices
    const int i = arg_tile.range().lobound()[2];
    const int j = arg_tile.range().lobound()[3];

    Matrix pno_ij = pnos[i*nocc_act + j];

    auto npno = pno_ij.cols();
    Matrix F_pno = pno_ij.transpose() * F_uocc * pno_ij;

    Vector eps_o = F_occ_act.diagonal();
    Vector eps_v = F_pno.diagonal();

    auto norm = 0.0;

    const auto e_i = eps_o[i];
    const auto e_j = eps_o[j];
    const auto e_ij = -e_i - e_j;

    // Loop over a and b indices to form T
    for (int a = 0, tile_idx = 0; a != npno; ++a) {
      const auto e_a = eps_v[a];

      for (int b = 0; b != npno; ++b, ++tile_idx) {
        const auto e_b = eps_v[b];
        const auto e_ab = e_a + e_b;

        const auto e_abij = e_ab + e_ij;
        // const auto e_abij = e_a + e_b - e_i - e_j;
        const auto K_abij = arg_tile[tile_idx];
        const auto T_abij = -K_abij / e_abij;
        const auto abs_result = std::abs(T_abij);
        norm += abs_result * abs_result;
        result_tile[tile_idx] = T_abij;

      }      // b
    }        // a
    return std::sqrt(norm);
  };  // form_T

  auto T2 = TA::foreach(K, form_T);
  T2.world().gop.fence();
  return T2;
};  // form T from K

template <typename Tile, typename Policy>
TA::DistArray<Tile, Policy> construct_density(
    const TA::DistArray<Tile, Policy>& t2) {
  using Matrix = RowMatrix<typename Tile::numeric_type>;
  /// Step (2): Transform T to D
  // lambda function to transform T to D; implement using a for_each
  std::size_t nuocc = t2.trange().dim(0).extent();
  auto form_D = [nuocc](Tile& result_tile, const Tile& arg_tile) {

    result_tile = Tile(arg_tile.range());

    // Get values of i,j and compute delta_ij
    const int i = arg_tile.range().lobound()[2];
    const int j = arg_tile.range().lobound()[3];

    double delta_ij = (i == j) ? 1.0 : 0.0;

    // Form T_ij matrix from arg_tile
    Matrix T_ij = TA::eigen_map(arg_tile, nuocc, nuocc);

    // Form T_ji matrix from T_ij
    Matrix T_ji = T_ij.transpose();

    // Form D_ij from T_ij and T_ji
    Matrix D_ij =
        (1.0 / (1.0 + delta_ij)) * (4.0 * T_ji * T_ij - 2.0 * T_ij * T_ij +
            4.0 * T_ij * T_ji - 2.0 * T_ji * T_ji);

    // Transform D_ij into a tile
    auto norm = 0.0;
    std::size_t tile_idx = 0;
    for (std::size_t a = 0; a < nuocc; ++a) {
      for (std::size_t b = 0; b < nuocc; ++b, ++tile_idx) {
        const auto elem = D_ij(a, b);
        norm += elem * elem;
        result_tile[tile_idx] = elem;
      }
    }
    return std::sqrt(norm);
  };  // form_D

  auto D = TA::foreach (t2, form_D);
  t2.world().gop.fence();

  return D;
};

/**
 *
 * @tparam Tile
 * @tparam Policy
 * @param D             pair density
 * @param F_uocc        unocc-unocc portion of the Fock matrix
 * @param tpno          PNO truncation threshold
 * @param tosv          OSV truncation threshold
 * @param pnos          vector of PNO matrices
 * @param npnos         vector of nPNO values
 * @param F_pno_diag    vector of Vectors, where each Vector contains the diagonal elements of
 *                      the PNO-transformed Fock matrix
 * @param osvs          vector of OSV matrices
 * @param nosvs         vector of nOSV values
 * @param F_osv_diag    vector of Vectors, where each Vector contains the diagonal elements of
 *                      the OSV-transformed Fock matrix
 * @param pno_canonical whether or not to canonicalize the PNOs
 */
template <typename Tile, typename Policy>
void construct_pno(
    const TA::DistArray<Tile, Policy>& D,
    const RowMatrix<typename Tile::numeric_type>& F_uocc,
    double tpno,
    double tosv,
    std::vector<RowMatrix<typename Tile::numeric_type>>& pnos,
    std::vector<int>& npnos,
    std::vector<int>& old_npnos,
    std::vector<EigenVector<typename Tile::numeric_type>>& F_pno_diag,
    std::vector<RowMatrix<typename Tile::numeric_type>>& osvs,
    std::vector<int>& nosvs,
    std::vector<int>& old_nosvs,
    std::vector<EigenVector<typename Tile::numeric_type>>& F_osv_diag,
    std::vector<EigenVector<typename Tile::numeric_type>>& pno_eigvals,
    bool pno_canonical = false,
    PNORankUpdateMethod update_pno_rank = PNORankUpdateMethod::standard) {
  using Matrix = RowMatrix<typename Tile::numeric_type>;

  auto& world = D.world();
  std::size_t nocc_act = D.trange().dim(2).extent();
  std::size_t nuocc = D.trange().dim(0).extent();

  // If npnos already initialized, set old_npnos = npnos
  // and old_nosvs = nosvs
  const bool have_old_pnos = (npnos.size() != 0);
  if(have_old_pnos) {
    old_npnos = npnos;
    old_nosvs = nosvs;
  }
  else {
    // Initialize old_npnos_ to have all zeroes
    old_npnos.resize(nocc_act * nocc_act);
    std::fill(old_npnos.begin(), old_npnos.end(), 0);
    // Initialize old_nosvs to have all zeroes
    old_nosvs.resize(nocc_act);
    std::fill(old_nosvs.begin(), old_nosvs.end(), 0);
  }

  // For storing PNOs and and the Fock matrix in the PNO basis
  npnos.resize(nocc_act * nocc_act);
  pnos.resize(nocc_act * nocc_act);
  F_pno_diag.resize(nocc_act * nocc_act);
  std::fill(npnos.begin(), npnos.end(), 0);

  // For storing the PNO eigenvalues
  pno_eigvals.resize(nocc_act * nocc_act);


  // For storing OSVs (PNOs when i = j) and the Fock matrix in
  // the OSV basis
  nosvs.resize(nocc_act);
  osvs.resize(nocc_act);
  F_osv_diag.resize(nocc_act);
  std::fill(nosvs.begin(), nosvs.end(), 0);

  // Lambda function to form osvs
  auto form_OSV = [&D, &F_osv_diag,
                   &F_uocc, &nosvs, &old_nosvs, tosv, nuocc, nocc_act,
                   pno_canonical, update_pno_rank, have_old_pnos](TA::World& world){

    Eigen::SelfAdjointEigenSolver<Matrix> es;

    std::vector<Matrix> osvs_list(nocc_act);

    auto reblock_r1_pmap = Policy::default_pmap(world, nocc_act);

    for (ProcessID r = 0; r < world.size(); ++r) {
      std::vector<int> local_tiles;
      world.gop.fence();
      if (r == world.rank()) {
        for (TiledArray::Pmap::const_iterator it = reblock_r1_pmap->begin();
             it != reblock_r1_pmap->end(); ++it){
          local_tiles.push_back(*it);
        }

        for (int idx = 0; idx != local_tiles.size(); ++idx) {
          auto i = local_tiles[idx];

          Tile D_ii = D.find({0,0,i,i}).get();
          Matrix D_ii_mat = TA::eigen_map(D_ii, nuocc, nuocc);


          // Diagonalize D_ii_mat
          es.compute(D_ii_mat);
          Matrix osv_ii = es.eigenvectors();
          auto occ_ii = es.eigenvalues();

          // Truncate osvs_list
          size_t osvdrop = 0;
          if (tosv != 0.0) {
            for (size_t k = 0; k != occ_ii.rows(); ++k) {
              if (!(occ_ii(k) >= tosv))
                ++osvdrop;
              else
                break;
            }
          }

          // Calculate the number of OSVs kept in this macro iteration
          auto nosv = nuocc - osvdrop;

          // If update_pno_rank == fuzzy, compare the would-be dropped OSVs to the fuzzy cutoff
          if (update_pno_rank == PNORankUpdateMethod::fuzzy) {
            const auto old_nosv = old_nosvs[i];

            int new_osvdrop = osvdrop;

            // Compare new npno_ij to old npno_ij
            if (nosv < old_nosv) {
              auto diff = old_nosv - nosv;
              for (int i = 1; i <= diff; ++i) {
                int idx = osvdrop - i;
                if (occ_ii(idx) >= tosv / 2.0) {
                  --new_osvdrop;
                } //if
              } // for
            } // if

            // Recompute the current macro iteration's npnos
            osvdrop = new_osvdrop;
            nosv = nuocc - osvdrop;

          }  // fuzzy update
          else if (update_pno_rank == PNORankUpdateMethod::fixed && have_old_pnos) {   // no update
            const auto old_nosv = old_nosvs[i];
            nosv = old_nosv;
            osvdrop = nuocc - nosv;
          }   // no update

          // Store the number of OSVs kept for computing the average later
          nosvs[i] = nosv;

          if (nosv == 0) {  // all OSV truncated indicates total nonsense
            throw ProgrammingError("# OSVs = 0 should not be possible", __FILE__, __LINE__);
          }

          // Store truncated OSV mat in osvs_list
          Matrix osv_trunc = osv_ii.block(0, osvdrop, nuocc, nosv);
          osvs_list[i] = osv_trunc;

          // Transform F to OSV space
          Matrix F_osv_i = osv_trunc.transpose() * F_uocc * osv_trunc;

          // Store just the diagonal elements of F_osv_i
          F_osv_diag[i] = F_osv_i.diagonal();

          // Transform osvs_list to canonical osvs_list if pno_canonical_ == true
          if (pno_canonical) {
            // Compute eigenvectors of F in OSV space
            es.compute(F_osv_i);
            Matrix osv_transform_i = es.eigenvectors();

            // Transform osv_i to canonical OSV space: osv_i -> can_osv_i
            Matrix can_osv_i = osv_trunc * osv_transform_i;

            // Replace standard with canonical osvs_list
            osvs_list[i] = can_osv_i;
            F_osv_diag[i] = es.eigenvalues();
          }  // pno_canonical
        }
      }
    }
    return osvs_list;
  };

  osvs = form_OSV(world);
  // The following fence is unnecessary IF OSVs are only used locally
  // world.gop.fence();

  // Lambda function to form PNOs; implement using a for_each
  auto form_PNO = [&pnos, &F_pno_diag, &F_uocc, &npnos, &old_npnos,
                   tpno, nuocc, nocc_act,
                   pno_canonical, &pno_eigvals, update_pno_rank, have_old_pnos]
      (Tile& result_tile, const Tile& arg_tile) {

    Eigen::SelfAdjointEigenSolver<Matrix> es;

    result_tile = Tile(arg_tile.range());

    // Get values of i,j and compute ij
    const int i = arg_tile.range().lobound()[2];
    const int j = arg_tile.range().lobound()[3];
    const int ij = i * nocc_act + j;

    // Form D_ij matrix from arg_tile
    Matrix D_ij = TA::eigen_map(arg_tile, nuocc, nuocc);

    // Only form PNOs for i <= j
    if (i <= j) {
      // Diagonalize D_ij
      es.compute(D_ij);
      Matrix pno_ij = es.eigenvectors();
      auto occ_ij = es.eigenvalues();

      pno_eigvals[ij] = occ_ij;

      // Determine number of PNOs to be dropped
      std::size_t pnodrop = 0;

      if (tpno != 0.0) {
        for (std::size_t k = 0; k != occ_ij.rows(); ++k) {
          if (!(occ_ij(k) >= tpno))
            ++pnodrop;
          else
            break;
        }
      }

      // Calculate the number of PNOs kept in this macro iteration
      auto npno = nuocc - pnodrop;

      // If update_pno_rank == fuzzy, compare the would-be dropped PNOs to the fuzzy cutoff
      if (update_pno_rank == PNORankUpdateMethod::fuzzy) {
        const auto old_npno = old_npnos[ij];

        int new_pnodrop = pnodrop;

        // Compare new npno_ij to old npno_ij
        if (npno < old_npno) {
          auto diff = old_npno - npno;
          for (int i = 1; i <= diff; ++i) {
            int idx = pnodrop - i;
            if (occ_ij(idx) >= tpno / 2.0) {
              --new_pnodrop;
            } //if
          } // for
        } // if

        // Recompute the current macro iteration's npnos
        pnodrop = new_pnodrop;
        npno = nuocc - pnodrop;

      }   // fuzzy update
      else if (update_pno_rank == PNORankUpdateMethod::fixed && have_old_pnos) {  // no update
        const auto old_npno = old_npnos[ij];
        npno = old_npno;
        pnodrop = nuocc - npno;
      }  // nu update


      // Store the new number of PNOs kept for calculating the average later
      npnos[ij] = npno;

      // Truncate PNO matrix and store in pnos_

      // Declare matrix pno_trunc to hold truncated set of PNOs
      Matrix pno_trunc;

      // If npno = 0, substitute a single fake "PNO" with all coefficients
      // equal to zero. All other code will behave the same way
      if (npno == 0) {
        // resize pno_trunc to be size nocc_act x 1
        pno_trunc.resize(nuocc, 1);

        // Create a zero matrix of size nuocc x 1
        Matrix pno_zero = Matrix::Zero(nuocc, 1);

        // Set pno_trunc eqaul to pno_zero matrix
        pno_trunc = pno_zero;
      }

        // If npno != zero, use actual zet of truncated PNOs
      else {
        pno_trunc.resize(nuocc, npno);
        pno_trunc = pno_ij.block(0, pnodrop, nuocc, npno);
      }

      // Store truncated PNOs
      pnos[ij] = pno_trunc;

      // Transform F to PNO space
      Matrix F_pno_ij = pno_trunc.transpose() * F_uocc * pno_trunc;

      // Store just the diagonal elements of F_pno_ij
      F_pno_diag[ij] = F_pno_ij.diagonal();

      // Transform PNOs to canonical PNOs if pno_canonical_ == true

      if (pno_canonical && npno > 0) {
        // Compute eigenvectors of F in PNO space
        es.compute(F_pno_ij);
        Matrix pno_transform_ij = es.eigenvectors();

        // Transform pno_ij to canonical PNO space; pno_ij -> can_pno_ij
        Matrix can_pno_ij = pno_trunc * pno_transform_ij;

        // Replace standard with canonical PNOs
        pnos[ij] = can_pno_ij;
        F_pno_diag[ij] = es.eigenvalues();
      }  // pno_canonical
    } // i <= j



    // Transform D_ij into a tile
    auto norm = 0.0;
    for (int a = 0, tile_idx = 0; a != nuocc; ++a) {
      for (int b = 0; b != nuocc; ++b, ++tile_idx) {
        const auto elem = D_ij(a, b);
        const auto abs_result = std::abs(elem);
        norm += abs_result;
        result_tile[tile_idx] = elem;
      }
    }
    return std::sqrt(norm);
  };  // form_PNO

  auto D_prime = TA::foreach (D, form_PNO);
  world.gop.fence();


}  // construct_pno

}  // namespace detail


// JacobiDIISSolver updates the CC T amplitudes using standard Jacobi+DIIS
template <typename T>
class JacobiDIISSolver : public ::mpqc::cc::DIISSolver<T> {
 public:
  // clang-format off
  /**
   * @brief The KeyVal constructor.
   *
   * @param kv the KeyVal object; it will be queried for all keywords of ::mpqc::cc::DIISSolver<T,T> .
   */
  // clang-format on

  JacobiDIISSolver(const KeyVal& kv,
                   Eigen::Matrix<double, Eigen::Dynamic, 1> f_ii,
                   Eigen::Matrix<double, Eigen::Dynamic, 1> f_aa)
      : ::mpqc::cc::DIISSolver<T>(kv) {
    std::swap(f_ii_, f_ii);
    std::swap(f_aa_, f_aa);
  }
  virtual ~JacobiDIISSolver() = default;

 private:
  Eigen::Matrix<double, Eigen::Dynamic, 1> f_ii_;
  Eigen::Matrix<double, Eigen::Dynamic, 1> f_aa_;

  void update_only(T& t1, T& t2, const T& r1, const T& r2, double dE) override {
    t1("a,i") += detail::jacobi_update_t1_ai(r1, f_ii_, f_aa_)("a,i");
    t2("a,b,i,j") += detail::jacobi_update_t2_abij(r2, f_ii_, f_aa_)("a,b,i,j");
    t1.truncate();
    t2.truncate();
  }

  void update_only(T& t1, T& t2, T& t3, const T& r1, const T& r2, const T& r3) override {
    t1("a,i") += detail::jacobi_update_t1_ai(r1, f_ii_, f_aa_)("a,i");
    t2("a,b,i,j") += detail::jacobi_update_t2_abij(r2, f_ii_, f_aa_)("a,b,i,j");
    t3("a,b,c,i,j,k") += detail::jacobi_update_t3_abcijk(r3, f_ii_, f_aa_)("a,b,c,i,j,k");
    t1.truncate();
    t2.truncate();
    t3.truncate();
  }

};


/// PNOSolver updates the CC T amplitudes using standard Jacobi+DIIS in PNO
/// space
/// @warning This class assumes that the 1- and 2-body amplitudes/residuals
///          given to Solver::update() are laid out as "a,i" and "a,b,i,j",
///          respectively
template <typename T, typename DT>
class PNOSolver : public ::mpqc::cc::DIISSolver<T>,
                  public madness::WorldObject<PNOSolver<T, DT>> {
  std::wstring formula(const wchar_t* f) const {
    return std::wstring(f) + (use_df_ ? L"[df]" : L"");
  }

 public:
  using Tile = typename T::value_type;
  using Matrix = RowMatrix<typename Tile::numeric_type>;
  using Vector = EigenVector<typename Tile::numeric_type>;

  using WorldObject_ = madness::WorldObject<PNOSolver<T, DT>>;

  // clang-format off
  /**
   * @brief The KeyVal constructor.
   *
   * @param kv the KeyVal object; it will be queried for all keywords of ::mpqc::cc::DIISSolver , as well
   * as the following additional keywords:
   *
   * | Keyword | Type | Default| Description |
   * |---------|------|--------|-------------|
   * | @c tpno | double | 1e-7 | The PNO construction threshold. This non-negative integer specifies the screening threshold for the eigenvalues of the pair density. Setting this to zero will cause the full (untruncated) set of PNOs to be used. |
   * | @c tosv | double | 1e-9 | The OSV construction threshold. This non-negative integer specifies the screening threshold for the eigenvalues of the pair density of the diagonal pairs. Setting this to zero will cause the full (untruncated) set of OSVs to be used. |
   * | @c pno_canonical | bool | false | Whether or not to canonicalize the PNOs and OSVs |
   * | @c pno_guess | string | scmp1 | How to construct the (initial) PNOs; valid values are "scmp1" (semicanonical MP1 amplitudes; exact if using canonical orbitals) and "mp1" (exact MP1 amplitudes) |
   * | @c update_pno | bool | false | Whether or not to recompute the PNOs |
   * | @c min_micro | int | 3 | The minimum number of micro iterations to perform per macro iteration |
   * | @c print_npnos | bool | false | Whether or not to print out nPNOs/pair every time PNOs are updated |
   * | @c micro_ratio | double | 3.0 | How much more tightly to converge w/in a macro iteration
   */
  // clang-format on
  PNOSolver(const KeyVal& kv, Factory<T, DT>& factory)
      : ::mpqc::cc::DIISSolver<T>(kv),
        madness::WorldObject<PNOSolver<T, DT>>(factory.world()),
        factory_(factory),
        use_df_(kv.value<bool>("use_df", true)),
        pno_canonical_(kv.value<bool>("pno_canonical", false)),
        pno_guess_(kv.value<std::string>("pno_guess", "scmp1", [](auto& arg) { return arg == "scmp1" || arg == "mp1"; }) ),
        update_pno_(kv.value<bool>("update_pno", false)),
        tpno_(kv.value<double>("tpno", 1.e-7)),
        tosv_(kv.value<double>("tosv", 1.e-9)),
        min_micro_(kv.value<int>("min_micro", 3)),
        print_npnos_(kv.value<bool>("print_npnos", false)),
        micro_ratio_(kv.value<double>("micro_ratio", 3.0)),
        update_pno_mixing_(kv.value<double>("update_pno_mixing", 0.0, "old_coeff", [](auto coeff){ return coeff >= 0.0 && coeff <= 1.0; })),
        update_pno_rank_(kv.exists("update_pno_rank") ? (kv.value<bool>("update_pno_rank", false, "use_fuzzy") ? PNORankUpdateMethod::fuzzy : PNORankUpdateMethod::fixed) : PNORankUpdateMethod::standard) {

    // compute and store PNOs truncated with threshold tpno_
    // store PNOs for diagonal pair as OSVs truncated with threshold tosv_

    auto& fac = factory_;
    auto& world = fac.world();
    auto& ofac = fac.orbital_registry();

    auto nocc = ofac.retrieve("m").rank();
    auto nocc_act = ofac.retrieve("i").rank();
    nocc_act_ = nocc_act;
    auto nuocc = ofac.retrieve("a").rank();
    nuocc_ = nuocc;
    auto nfzc = nocc - nocc_act;
    iter_count_ = 0;

    // Counts how many micro iterations per macro iteration
    // In subsequent macro iterations, this will start at 1
    micro_count_ = 0;


    // Whether or not PNOs have been updated at least once
    pnos_relaxed_ = false;

    // Set start_macro_ to false to begin with, even though this is technically the start of a macro iteration
    start_macro_ = false;

    E_11_ = 0.0;
    E_12_ = 0.0;
    E_21_ = 0.0;
    E_22_ = 0.0;

    // Form Fock array
    auto F = fac.compute(formula(L"<p|F|q>"));

    // Transform entire Fock array to Eigen Matrix
    Matrix F_all = math::array_to_eigen(F);

    // Select just diagonal elements of Fock aray and transform
    // to Eigen vector; use for computing PNOs
    Vector eps_p = F_all.diagonal();
    auto eps_o = eps_p.segment(nfzc, nocc_act);
    auto eps_v = eps_p.tail(nuocc);

    // Select just the occupied portion of the Fock matrix
    F_occ_act_ = F_all.block(nfzc, nfzc, nocc_act, nocc_act);

    // Select just the unoccupied portion of the Fock matrix
    F_uocc_ = F_all.block(nocc, nocc, nuocc, nuocc);


    // Compute all K_aibj
    K = fac.compute(formula(L"<a b|G|i j>"));
    const auto ktrange = K.trange();

    // Create TiledRange1 objects for uocc transformation arrays
    std::vector<std::size_t> uocc_blocks{0, nuocc};

    const TA::TiledRange1 uocc_col =
        TA::TiledRange1(uocc_blocks.begin(), uocc_blocks.end());
    const TA::TiledRange1 uocc_row = ktrange.dim(0);

    // Create TiledRange1 objects for occ transformation arrays
    std::vector<std::size_t> occ_blocks;
    for (std::size_t i = 0; i <= nocc_act_; ++i) {
      occ_blocks.push_back(i);
    }

    const TA::TiledRange1 occ_col =
        TA::TiledRange1(occ_blocks.begin(), occ_blocks.end());
    const TA::TiledRange1 occ_row = ktrange.dim(3);

    // compute T2 amplitudes
    auto T2 = (pno_guess_ == "scmp1") ? compute_canon_mp1_t2(K, eps_o, eps_v) : compute_mp1_t2(K, fac, ofac);

    // Form exact MP2 correlation energy for use in computing PNO incompleteness correction
    exact_e_mp2_ = detail::compute_mp2(K, T2);
    ExEnv::out0() << "Exact MP2 correlation energy: " << exact_e_mp2_ << std::endl;

    // Create transition arrays for reblocking
    reblock_a_ = mpqc::math::create_diagonal_array_from_eigen<
        Tile, TA::detail::policy_t<T>>(world, uocc_row, uocc_col, 1.0);

    reblock_i_ = mpqc::math::create_diagonal_array_from_eigen<
        Tile, TA::detail::policy_t<T>>(world, occ_row, occ_col, 1.0);

    // Reblock K and T
    K_reblock_ = detail::reblock_t2(K, reblock_i_, reblock_a_);
    T T_reblock = detail::reblock_t2(T2, reblock_i_, reblock_a_);

    // Construct PNOs and OSVs
    auto D = detail::construct_density(T_reblock);
    old_D_ = D; // Save this original D in old_D_ for purposes of mixing
    detail::construct_pno(D, F_uocc_,
                          tpno_, tosv_,
                          pnos_, npnos_, old_npnos_, F_pno_diag_,
                          osvs_, nosvs_, old_nosvs_, F_osv_diag_, pno_eigvals_, pno_canonical_, update_pno_rank_);

    // ready to process tasks now
    this->process_pending();

    // Transfer PNOs to the node that owns them
    transfer_pnos();

    // Print ave nPNOs/pair
    print_ave_npnos_per_pair();

    // Compute PNO-MP2 correction
    compute_pno_mp2_correction();

    // Dump # of pnos/pair to file
    if (print_npnos_) {
      print_npnos_per_pair();
      print_eigvals();
    }

  }

  virtual ~PNOSolver() = default;

  /// @return PNO truncation threshold
  double tpno() const { return tpno_; }
  /// @return OSV truncation threshold
  double tosv() const { return tosv_; }

  const auto& pno(int i, int j) const { return pnos_[i * nocc_act_ + j]; }
  const auto& osv(int i) const { return osvs_[i]; }

  const auto& npnos(int i, int j) const { return npnos_[i * nocc_act_ + j]; }

 private:
  /// Overrides DIISSolver::is_converged()
  bool is_converged(double target_precision, double error,
                    double dE) const override {
    // dE = std::abs(E_22_ - E_21_): the difference in energy between the two
    // most recent micro iterations DE = std::abs(E_22_ - E_12_): the difference
    // in energy between the current micro iteration and the last micro
    // iteration of the previous macro iteration

    const auto pessimistic_target_precision = target_precision * this->precision_margin_energy();

    if (update_pno_ == true) {
      const auto DE = std::abs(E_22_ - E_12_);
      return (pnos_relaxed_ &&
              dE < pessimistic_target_precision &&
              DE < pessimistic_target_precision);
    } else {
      return dE < pessimistic_target_precision;
    }
  }

  bool is_converged_within_subspace(double dE) {
    if (update_pno_ == true) {
      const auto DE = std::abs(E_22_ - E_12_);

      // Check that dE satisfies the following requirement
      bool energy_check = pnos_relaxed_ ? (dE < DE / micro_ratio_)
                                        : (dE < std::abs(E_22_) / 1.0e-04);

      // Check that micro_count_ is greater than or equal to min_micro_
      bool micro_check = micro_count_ >= min_micro_;

      return (energy_check && micro_check);
    }

    else {
      return false;
    }
  }

  /// Overrides DIISSolver::update_only() .
  /// @note must override DIISSolver::update() also since the update must be
  ///      followed by backtransform updated amplitudes to the full space
  void update_only(T& t1, T& t2, const T& r1, const T& r2, double E1) override {

    T delta_t1_ai;
    T delta_t2_abij;

    // Perform Jacobi update in full space when PNOs being recomputed
    if ((update_pno_ == true) && (start_macro_ == true)) {
      Vector ens_occ_act = F_occ_act_.diagonal();
      Vector ens_uocc = F_uocc_.diagonal();

      delta_t1_ai =
        detail::jacobi_update_t1_ai(r1, ens_occ_act, ens_uocc);
      delta_t2_abij =
        detail::jacobi_update_t2_abij(r2, ens_occ_act, ens_uocc);
    }

    // Perform Jacobi update in PNO space on specified iterations
    else {
      Vector ens_occ_act = F_occ_act_.diagonal();
      delta_t1_ai =
        detail::pno_jacobi_update_t1(r1, ens_occ_act, F_osv_diag_, osvs_);
      delta_t2_abij =
        detail::pno_jacobi_update_t2(r2, ens_occ_act, F_pno_diag_, pnos_);
    }


    // Reblock delta_t1_ai and delta_t2_abij to match original tiling
    auto delta_t1 = detail::unblock_t1(delta_t1_ai, reblock_i_, reblock_a_);
    auto delta_t2 = detail::unblock_t2(delta_t2_abij, reblock_i_, reblock_a_);
    t1("a,i") += delta_t1("a,i");
    t2("a,b,i,j") += delta_t2("a,b,i,j");
    t1.truncate();
    t2.truncate();

  }

  void update(T& t1, T& t2, const T& r1, const T& r2, double E1) override {

    // E_22_ set to current energy
    E_22_ = E1;

    // Compute dE from the current energy and the previous micro iteration of the same macro iteration
    double deltaE = std::abs(E_22_ - E_21_);  //!< Equivalent to dE in the definition of is_converged()

    // Check to see if energy has converged within current subspace
    if (update_pno_ && is_converged_within_subspace(deltaE)) {
      start_macro_ = true;  //!< Time to recompute PNOs
      E_11_ = E_21_;        //!< Current macro iter becomes previous macro iter
      E_12_ = E_22_;        //!< Current macro iter becomes previuos macro iter
    }

    // Redefine the energy of the previous micro iteration of the current macro iteration
    E_21_ = E1;


    // Reblock r1 and r2
    T r2_reblock = detail::reblock_t2(r2, reblock_i_, reblock_a_);
    T r1_reblock = detail::reblock_t1(r1, reblock_i_, reblock_a_);

    // Create copy of pre-update T2 for recomputing PNOs without adding Delta
    T t2_old = t2;
    T t2_old_reblock = detail::reblock_t2(t2_old, reblock_i_, reblock_a_);


    // Whether or not Jacobi update is performed in PNO subspace
    // or full space will be determined within update_only
    update_only(t1, t2, r1_reblock, r2_reblock, E1);


    // Recompute PNOs when start_macro_ == true
    if ((update_pno_ == true) && (start_macro_ == true)) {

      // We only compute the max principal angle between PNO subspaces when nproc = 1
      if (this->get_world().size() == 1) {
        old_pnos_ = pnos_;
      }


      // Construct new D
      T T_reblock = detail::reblock_t2(t2, reblock_i_, reblock_a_);
      auto D = detail::construct_density(T_reblock);

      // Mix old_D and new D
      T mixed_D;
      mixed_D("a,b,i,j") = update_pno_mixing_ * old_D_("a,b,i,j") + (1 - update_pno_mixing_) * D("a,b,i,j");

      // Set old_D_ equal to mixed_D for next macro iteration's mixing
      old_D_ = mixed_D;

      // Recompute the PNOs using mixed_D
      detail::construct_pno(mixed_D, F_uocc_,
                            tpno_, tosv_,
                            pnos_, npnos_, old_npnos_, F_pno_diag_,
                            osvs_, nosvs_, old_nosvs_, F_osv_diag_, pno_eigvals_, pno_canonical_, update_pno_rank_);

      // Once PNOs have been recomputed at least once, pnos_relaxed_ becomes true
      pnos_relaxed_ = true;

      // Copy PNOs to node that owns them
      transfer_pnos();

      // Dump # of pnos/pair to file
      if (print_npnos_) {
        print_npnos_per_pair();
        print_eigvals();
      }

      // Transform t_old_reblock
      T T2 = detail::t2_project_pno(t2_old_reblock, pnos_);
      t2 = detail::unblock_t2(T2, reblock_i_, reblock_a_);


      mpqc::cc::TPack<T> r(r1_reblock, r2_reblock);
      mpqc::cc::TPack<T> t(t1, t2);
      this->reset();
      iter_count_ += 1;
      start_macro_ = false;  //!< set start_macro_ to false since next CCSD iter is in PNO subspace just entered
      micro_count_ = 1;      //!< reset count of micro iterations in the new subspace
    }

    else {
      // Print right before first micro iteration of the macro iteration is logged for
      // all macro iterations after the first one
      if (update_pno_ && pnos_relaxed_ && micro_count_ == 1) {
        // Print average number of PNOs per pair
        print_ave_npnos_per_pair();

        // Compute max principal angle between PNO subspaces when nproc = 1
        if (this->get_world().size() == 1) {
          compute_max_principal_angle();
        }

        // Compute PNO-MP2 correction
        compute_pno_mp2_correction();
      }

      // transform residuals to the PNO space for the sake of extrapolation
      T r1_osv = detail::osv_transform_ai(r1_reblock, osvs_);
      T r2_pno = detail::pno_transform_abij(r2_reblock, pnos_);
      mpqc::cc::TPack<T> r(r1_osv, r2_pno);
      mpqc::cc::TPack<T> t(t1, t2);

      this->diis().extrapolate(t, r);
      t1 = t.at(0);
      t2 = t.at(1);
      iter_count_ += 1;
      micro_count_ += 1;
    }

  }

 public:
  /// Overrides Solver<T,T>::error()
  virtual double error(const T& r1, const T& r2) override {

    auto r1_reblock = detail::reblock_t1(r1, reblock_i_, reblock_a_);
    auto r2_reblock = detail::reblock_t2(r2, reblock_i_, reblock_a_);

    detail::R1SquaredNormReductionOp<T> op1(osvs_);
    detail::R2SquaredNormReductionOp<T> op2(pnos_);

    auto residual = sqrt(r1_reblock("a,i").reduce(op1).get() +
                         r2_reblock("a,b,i,j").reduce(op2).get()) /
                         (size(r1_reblock) + size(r2_reblock));

    return residual;

  }

 private:

  /// Transfers the PNOs from the node that owns pair i,j to the node that owns pair j,i
  void transfer_pnos() {
    auto pmap = K_reblock_.pmap();
    auto tile_extent = K_reblock_.trange().tiles_range().extent_data();

    for(auto i = 0; i < tile_extent[2]; ++i) {
      for (auto j = i + 1; j < tile_extent[3]; ++j) {
        if(!K_reblock_.is_zero({0,0,i,j}) && K_reblock_.is_local({0,0,i,j})) {
          auto ji_owner = pmap->owner(j * nocc_act_ + i);
          const auto ij = i * nocc_act_ + j;
          WorldObject_::task(ji_owner, &PNOSolver::copy_pnoij, i, j, pnos_[ij], npnos_[ij], F_pno_diag_[ij], pno_eigvals_[ij]);
        }
      }
    }

    this->get_world().gop.fence();
  }

  /**
   * Copies the PNOs for pair i,j to pair j,i
   * @param i The first occupied index
   * @param j The second occupied index
   * @param pno_ij The Matrix of PNOs associated with pair i,j
   * @param npno_ij The number of PNOs axxociated with pair i,j
   * @param f_pno_diag_ij A Vector containing the diagonal elements of the Fock matrix
   *        transformed to the i,j PNO space
   */
  void copy_pnoij(int i, int j, Matrix pno_ij, int npno_ij, Vector f_pno_diag_ij, Vector pno_eigvals_ij) {
    int my_i = j;
    int my_j = i;
    int my_ij = my_i * nocc_act_ + my_j;
    pnos_[my_ij] = pno_ij;
    npnos_[my_ij] = npno_ij;
    F_pno_diag_[my_ij] = f_pno_diag_ij;
    pno_eigvals_[my_ij] = pno_eigvals_ij;
  }

  /// Prints the average number of PNOs and OSVs per pair
  void print_ave_npnos_per_pair() {

    this->get_world().gop.sum(npnos_.data(), npnos_.size());
    this->get_world().gop.sum(nosvs_.data(), nosvs_.size());

    if (this->get_world().rank() == 0) {
      auto tot_osv = 0;
      for (int i = 0; i != nosvs_.size(); ++i) {
        tot_osv += nosvs_[i];
      }

      auto ave_nosv = tot_osv / nocc_act_;

      // Compute and print average number of PNOs per pair
      auto tot_pno = 0;
      for (int i = 0; i != npnos_.size(); ++i) {
        tot_pno += npnos_[i];
      }

      auto ave_npno = tot_pno / (nocc_act_ * nocc_act_);

      ExEnv::out0() << "ave. nPNOs/pair: " << ave_npno << ", ave nOSVs/pair: " << ave_nosv << std::endl;
    }
  }

  /// Prints the number of PNOs for each occupied pair
  void print_npnos_per_pair() {
    std::string filename = FormIO::fileext_to_fullpathname("-npnos_iter-" + std::to_string(iter_count_) + ".tsv");
    std::ofstream out_file(filename);

    out_file << "i" << " " << "j" << " " << "nPNOs" << std::endl;

    for (int i = 0; i != nocc_act_; ++i) {
      for (int j = 0; j != nocc_act_; ++j) {
        int val = npnos_[i * nocc_act_ + j];
        out_file << i << " " << j << " " << val << std::endl;
      }
    }
  }

  /// Prints all eigenvalues for each (unique) occupied pair
  void print_eigvals() {

    for (int i = 0; i != nocc_act_; ++i) {
      for (int j = i; j != nocc_act_; ++j) {
        std::string filename = FormIO::fileext_to_fullpathname("-pno_eigvals_i-" + std::to_string(i) + "-j-" + std::to_string(j) + "-iter-" + std::to_string(iter_count_) + ".tsv");
        std::ofstream out_file(filename);
        auto ij = i * nocc_act_ + j;
        auto eigvals_ij = pno_eigvals_[ij];
        out_file << eigvals_ij << std::endl;
      }
    }
  }

  /// computes the MP1 T2 amplitudes by assuming canonical orbitals
  /// @return the semicanonical MP1 T2 amplitudes
  template <typename V1, typename V2> auto compute_canon_mp1_t2(const T& K, V1&& eps_o, V2&& eps_v) {
    // lambda function to convert K to T; implement using a for_each

    auto form_T = [&eps_o, &eps_v](Tile& result_tile, const Tile& arg_tile) {

      result_tile = Tile(arg_tile.range());

      // determine range of i and j indices
      const int i0 = arg_tile.range().lobound()[2];
      const int in = arg_tile.range().upbound()[2];
      const int j0 = arg_tile.range().lobound()[3];
      const int jn = arg_tile.range().upbound()[3];

      // determine range of a and b indices
      const int a0 = arg_tile.range().lobound()[0];
      const int an = arg_tile.range().upbound()[0];
      const int b0 = arg_tile.range().lobound()[1];
      const int bn = arg_tile.range().upbound()[1];

      auto norm = 0.0;

      // Loop over all four indices to form T
      for (int a = a0, tile_idx = 0; a != an; ++a) {
        const auto e_a = eps_v[a];

        for (int b = b0; b != bn; ++b) {
          const auto e_b = eps_v[b];
          const auto e_ab = e_a + e_b;

          for (int i = i0; i != in; ++i) {
            const auto e_i = eps_o[i];

            for (int j = j0; j != jn; ++j, ++tile_idx) {
              const auto e_j = eps_o[j];
              const auto e_ij = -e_i - e_j;

              const auto e_abij = e_ab + e_ij;
              // const auto e_abij = e_a + e_b - e_i - e_j;
              const auto K_abij = arg_tile[tile_idx];
              const auto T_abij = -K_abij / e_abij;
              const auto abs_result = std::abs(T_abij);
              norm += abs_result * abs_result;
              result_tile[tile_idx] = T_abij;
            }  // j
          }    // i
        }      // b
      }        // a
      return std::sqrt(norm);
    };  // form_T

    auto T2 = TA::foreach(K, form_T);
    this->get_world().gop.fence();

    return T2;
  }

  /// computes the MP1 T2 amplitudes by iteratively solving MP1 equations
  /// @return the MP1 T2 amplitudes
  template <typename Factory, typename OFactory> auto compute_mp1_t2(const T& K, Factory&& fac, OFactory&& ofac) {

    auto& world = fac.world();

    auto nocc = ofac.retrieve("m").rank();
    auto nocc_act = ofac.retrieve("i").rank();
    auto nvir = ofac.retrieve("a").rank();
    auto nfzc = nocc - nocc_act;

    auto F = fac.compute(formula(L"(p|F|q)"));
    Eigen::VectorXd eps_p = math::array_to_eigen(F).diagonal();
    // replicated diagonal elements of Fo
    auto eps_o = eps_p.segment(nfzc, nocc_act);
    // replicated diagonal elements of Fv
    auto eps_v = eps_p.tail(nvir);

    // Fij
    auto Fo = fac.compute(formula(L"(i|F|j)"));
    // Fab
    auto Fv = fac.compute(formula(L"(a|F|b)"));

    // zero out amplitudes
    auto T2 = T(world, K.trange(), K.shape());
    T2.fill(0.0);

    // lambda function will be used to do a Jacobi update of the residual
    auto jacobi_update = [eps_o, eps_v](TA::TensorD& result_tile) {

      const auto& range = result_tile.range();
      double norm = 0.0;
      for (const auto& i : range) {
        const auto result_abij = result_tile[i] / (eps_o[i[2]] + eps_o[i[3]]
            - eps_v[i[0]] - eps_v[i[1]]);
        result_tile[i] = result_abij;
        norm += result_abij * result_abij;
      }
      return std::sqrt(norm);
    };

    // solve the MP1 equations
    auto converged = false;
    auto iter = 0;
    auto energy = +1.0;
    const auto target_precision = 1e-6;
    ExEnv::out0() << "MP1 T2 Solver\n";
    while (not converged) {
      T R;
      R("a,b,i,j") = K("a,b,i,j") + Fv("a,c") * T2("c,b,i,j") +
          Fv("b,c") * T2("a,c,i,j") - Fo("i,k") * T2("a,b,k,j") -
          Fo("j,k") * T2("a,b,i,k");

      // estimate the MP2 energy ... the Hylleraas formula is quadratic in error
      double updated_energy =
          (K("a,b,i,j") + R("a,b,i,j")).dot(2 * T2("a,b,i,j") - T2("b,a,i,j"));
      ExEnv::out0() << indent << "Iteration: " << iter
                    << " Energy: " << updated_energy << std::endl;

      const auto computed_precision = std::abs(updated_energy - energy);
      energy = updated_energy;

      // update the amplitudes, if needed
      converged = computed_precision <= target_precision;
      if (not converged) {
        // R^{ij}_{ab} -> R^{ij}_{ab} / (F^i_i + F^j_j - F^a_a - F^b_b)
        TA::foreach_inplace(R, jacobi_update);
        // need a fence here since foreach_inplace mutates the contents of R
        // as a side effect.
        // N.B. most TiledArray ops will not need a fence (except to control
        //      the resource use)
        world.gop.fence();
        T2("a,b,i,j") += R("a,b,i,j");
        ++iter;
      }
    }

    return T2;
  }

  /// Computes the PNO-MP2 correction
  void compute_pno_mp2_correction() {
    // Form K_pno and T2_pno
    T K_pno = detail::pno_transform_abij(K_reblock_, pnos_);
    T T2_pno = detail::form_T_from_K(K_pno, F_occ_act_, F_uocc_, pnos_, nocc_act_);

    // Compute the MP2 energy in the space of the truncated PNOs
    auto pno_e_mp2 = detail::compute_mp2(K_pno, T2_pno);

    // Compute difference between the MP2 energy and the PNO-MP2 energy
    auto mp2_correction = exact_e_mp2_ - pno_e_mp2;


    ExEnv::out0() << "PNO-MP2 correlation energy: " << pno_e_mp2 << ", PNO-MP2 correction: " << mp2_correction << std::endl;
  }


  /// Computes the max principal angles between consecutive PNO subspaces
  void compute_max_principal_angle() {

    double max_angle = 0.0;
    int idxi = 0;
    int idxj = 0;

    for (int i = 0; i != nocc_act_; ++i) {
      for (int j = i; j != nocc_act_; ++j) {
        Matrix old_u = old_pnos_[i * nocc_act_ + j];
        Matrix new_u = pnos_[i * nocc_act_ + j];

        Matrix product = old_u.transpose() * new_u;

        Eigen::JacobiSVD<Matrix> svd(product);

        Vector sing_vals = svd.singularValues();
        int length = sing_vals.size();

        double smallest_sing_val = sing_vals[length - 1];

        double angle = acos(smallest_sing_val);

        if (angle > max_angle) {
          max_angle = angle;
          idxi = i;
          idxj = j;
        }
      }
    }

    ExEnv::out0() << "The max principal angle is " << max_angle
                  << " and occurs for pair i,j = " << idxi << ", " << idxj << std::endl;
  }



  Factory<T, DT>& factory_;
  bool use_df_;                //!< whether to use DF-based Fock matrix and 4-index integrals
  bool pno_canonical_;         //!< whether or not to canonicalize PNO/OSV
  std::string pno_guess_;      //!< how to construct the (initial) PNO/OSV
  bool update_pno_;            //!< whether or not to update PNOs
  double tpno_;                //!< the PNO truncation threshold
  double tosv_;                //!< the OSV (diagonal PNO) truncation threshold
  int nocc_act_;               //!< the number of active occupied orbitals
  int nuocc_;                  //!< the number of unoccupied orbitals
  T T_;                        //!< the array of MP1 T amplitudes

  int iter_count_;             //!< the CCSD iteration

  T reblock_i_;                //!< The array used to reblock the occ dimension
  T reblock_a_;                //!< The array used to reblock the unocc dimension

  T K;                         //!< the T2 like array of eris; has dimensions "a,b,i,j"
  T K_reblock_;                //!< the T2 like array of eris reblocked to have a single occupied elem per tile
  Matrix F_occ_act_;           //!< The occ_act-occ_act portion of the Fock matrix
  Matrix F_uocc_;              //!< The unocc-unocc portion of the Fock matrix

  double exact_e_mp2_;         //!< the exact MP2 correlation energy

  // For storing PNOs and and the Fock matrix in the PNO basis
  std::vector<int> npnos_;
  std::vector<Matrix> pnos_;
  std::vector<Vector> F_pno_diag_;
  std::vector<int> old_npnos_;
  std::vector<Matrix> old_pnos_;
  std::vector<Vector> pno_eigvals_;

  // For storing OSVs (PNOs when i = j) and the Fock matrix in
  // the OSV basis
  std::vector<int> nosvs_;
  std::vector<Matrix> osvs_;
  std::vector<Vector> F_osv_diag_;
  std::vector<int> old_nosvs_;
  std::vector<Matrix> old_osvs_;

  bool start_macro_;            //!< Indicates when a CCSD iteration is the first in a macro iteration
  bool pnos_relaxed_;           //!< Whether or not PNOs have been updated at least once

  int micro_count_;             //!< Keeps track of the number of micro iterations in the current macro iteration
  int min_micro_;               //!< The minimum number of micro iterations per macro iteration
  bool print_npnos_;            //!< Whether or not nPNOs for each pair should be printed
                                //!< and whether or not the pno eigenvalues should be printed

  double E_11_;                 //!< The energy of the second to last micro iteration of the previous macro iteration
  double E_12_;                 //!< The energy of the last micro iteration of the previous macro iteration
  double E_21_;                 //!< The energy of the previous micro iteration of the current macro iteration
  double E_22_;                 //!< The energy of the current micro iteration of the current macro iteration

  double micro_ratio_;          //!< For determining whether or not the energy has converged within a subspace

  T old_D_;                     //!< Holds the previous value of D for mixing purposes
  double update_pno_mixing_;    //!< Coefficient for old_D in mixing
  PNORankUpdateMethod update_pno_rank_;        //!< PNO rank update strategy:

};  // class: PNO solver

}  // namespace cc
}  // namespace lcao
}  // namespace mpqc

#endif /* SRC_MPQC_CHEMISTRY_QC_LCAO_CC_SOLVERS_H_ */
