//
// Created by ChongPeng on 2/20/17.
//

#ifndef SRC_MPQC_MATH_LINALG_DAVIDSON_DIAG_H_
#define SRC_MPQC_MATH_LINALG_DAVIDSON_DIAG_H_

#include <TiledArray/algebra/utils.h>
#include <tiledarray.h>

#include "mpqc/math/external/eigen/eigen.h"
#include "mpqc/math/linalg/gram_schmidt.h"
#include "mpqc/util/misc/assert.h"
#include "mpqc/util/misc/exception.h"
#include "mpqc/util/misc/exenv.h"

namespace mpqc {

// clang-format off
/**
 * \brief Davidson Algorithm
 *
 * Solves eigen value problem <tt> Hx = ex </tt> for the lowest n eigen value and eigen vector
 *
 * it starts with orthogonal guess vector B {b1, b2, ... bn}
 * the eigen vector is a linear combination of B
 * extrapolate() will update the vector B and store new x
 *
 * \tparam D
 * array type
 * \c D::element_type must be defined and \c D must provide the following stand-alone functions:
 * - `D copy(const D&)`
 * - `element_type dot_product(const D& a, const D& b)`
 * - `void scale(D& y , element_type a)`
 * - `void axpy(D&y , element_tye a, const D& z)`
 * - `void zero(D& x)`
 *
 */
// clang-format on
template <typename D>
class DavidsonDiag {
 public:
  using element_type = typename D::element_type;
  using result_type = EigenVector<element_type>;
  using value_type = std::vector<D>;

 private:
  struct EigenPair {
    element_type eigen_value;
    result_type eigen_vector;

    /// constructor
    EigenPair(const element_type& value, const result_type& vector)
        : eigen_value(value), eigen_vector(vector) {}

    /// move constructor
    EigenPair(const element_type&& value, const result_type&& vector)
        : eigen_value(std::move(value)), eigen_vector(std::move(vector)) {}

    ~EigenPair() = default;

    // sort by eigen value
    bool operator<(const EigenPair& other) const {
      return eigen_value < other.eigen_value;
    }
  };

 public:
  /**
   *
   * @param n_roots number of lowest roots to solve
   * @param n_guess number of eigen vector per root at subspace collapse,
   * default is 2
   * @param max_n_guess max number of guess vector per root, default is 3
   * @param symmetric if matrix is symmetric
   */
  DavidsonDiag(unsigned int n_roots, bool symmetric = true,
               unsigned int n_guess = 2, unsigned int max_n_guess = 3)
      : n_roots_(n_roots),
        symmetric_(symmetric),
        n_guess_(n_guess),
        max_n_guess_(max_n_guess),
        eigen_vector_() {}

  ~DavidsonDiag() { eigen_vector_.clear(); }

  /// @return all stored eigen vector in Davidson
  std::deque<value_type, std::allocator<value_type>>& eigen_vector() {
    return eigen_vector_;
  }

  // clang-format off
  /**
   *
   * @tparam Pred preconditioner object, which has void Pred(const element_type & e,D& residual) to update residual
   *
   * @param HB product with A and guess vector
   * @param B  guess vector
   * @param pred preconditioner
   *
   * @return B updated guess vector
   * @return updated eigen values
   */
  // clang-format on
  template <typename Pred>
  EigenVector<element_type> extrapolate(value_type& HB, value_type& B,
                                        value_type& new_B, const Pred& pred) {
    TA_ASSERT(HB.size() == B.size());
    // size of vector
    const auto n_v = B.size();

    // subspace
    // G will be replicated Eigen Matrix
    RowMatrix<element_type> G(n_v, n_v);
    for (auto i = 0; i < n_v; ++i) {
      for (auto j = 0; j < n_v; ++j) {
        G(i, j) = dot_product(B[j], HB[i]);
      }
    }

//    std::cout << "G: " << std::endl;
//    std::cout << G << std::endl;

    // do eigen solve locally
    result_type E(n_roots_);
    RowMatrix<element_type> C(n_v, n_roots_);

    // symmetric matrix
    if (symmetric_) {
      // this return eigenvalue and eigenvector
      Eigen::SelfAdjointEigenSolver<RowMatrix<element_type>> es(G);

      RowMatrix<element_type> v = es.eigenvectors();
      EigenVector<element_type> e = es.eigenvalues();

      if (es.info() != Eigen::Success) {
        throw AlgorithmException("Eigen::SelfAdjointEigenSolver Failed!\n",
                                 __FILE__, __LINE__);
      }

      //        std::cout << es.eigenvalues() << std::endl;

      E = e.segment(0, n_roots_);
      C = v.leftCols(n_roots_);
    }
    // non-symmetric matrix
    else {
      // unitary transform to upper triangular matrix
      Eigen::RealSchur<RowMatrix<element_type>> rs(G);

      RowMatrix<element_type> T = rs.matrixT();
      RowMatrix<element_type> U = rs.matrixU();

      if (rs.info() != Eigen::Success) {
        throw AlgorithmException("Eigen::RealSchur Failed!\n", __FILE__,
                                 __LINE__);
      }

      // do eigen solve on T
      Eigen::EigenSolver<RowMatrix<element_type>> es(T);

      // sort eigen values
      std::vector<EigenPair> eg;
      {
        RowMatrix<element_type> v = es.eigenvectors().real();
        EigenVector<element_type> e = es.eigenvalues().real();

        if (rs.info() != Eigen::Success) {
          throw AlgorithmException("Eigen::EigenSolver Failed!\n", __FILE__,
                                   __LINE__);
        }

        for (auto i = 0; i < n_v; ++i) {
          eg.emplace_back(e[i], v.col(i));
        }

        std::sort(eg.begin(), eg.end());
      }

      // obtain final eigen value and eigen vector
      for (auto i = 0; i < n_roots_; ++i) {
        E[i] = eg[i].eigen_value;
        C.col(i) = U * eg[i].eigen_vector;
      }
    }

    // compute eigen_vector at current iteration and store it
    // X(i) = B(i)*C(i)
    value_type X(n_roots_);
    for (auto i = 0; i < n_roots_; ++i) {
      X[i] = copy(B[i]);
      zero(X[i]);
      for (auto j = 0; j < n_v; ++j) {
        axpy(X[i], C(j, i), B[j]);
      }
    }

    // check the size, if exceed n_guess, pop oldest
    if (eigen_vector_.size() == n_guess_) {
      eigen_vector_.pop_front();
    }
    eigen_vector_.push_back(X);

    // compute residual
    // R(i) = (H - e(i)I)*B(i)*C(i)
    //      = (HB(i)*C(i) - e(i)*X(i)
    value_type residual(n_roots_);
    for (auto i = 0; i < n_roots_; ++i) {
      residual[i] = copy(X[i]);
      const auto e_i = -E[i];
      scale(residual[i], e_i);
      for (auto j = 0; j < n_v; ++j) {
        axpy(residual[i], C(j, i), HB[j]);
      }
    }

    // precondition
    // user should define preconditioner
    // usually it is D(i) = (e(i) - H_D)^-1 R(i)
    // where H_D is the diagonal element of H
    // but H_D can be approximated and computed on the fly
    for (auto i = 0; i < n_roots_; i++) {
      pred(E[i], residual[i]);
    }

    // subspace collapse
    // restart with new vector and most recent eigen vector
    // Journal of Computational Chemistry, 11(10), 1164–1168.
    // https://doi.org/10.1002/jcc.540111008
    if (B.size() >= n_roots_ * max_n_guess_) {
      B.clear();
      HB.clear();
      new_B.clear();
      new_B.insert(new_B.end(), residual.begin(), residual.end());
      // use all stored eigen vector from last n_guess interation
      for (auto& vector : eigen_vector_) {
        new_B.insert(new_B.end(), vector.begin(), vector.end());
      }
      // orthognolize all vectors
      gram_schmidt(new_B);
      // call it second times
      gram_schmidt(new_B);
      // update guess vector
      B = new_B;
    } else {
      // TODO better way to orthonormalize than double gram_schmidt
      // orthognolize new residual with original B
      gram_schmidt(B, residual);
      // call it twice
      gram_schmidt(B, residual);
      new_B = residual;
      // update guess vector
      B.insert(B.end(), new_B.begin(), new_B.end());
    }

// test if orthonomalized
#ifndef NDEBUG
    const auto k = B.size();
    const auto tolerance =
        std::numeric_limits<typename D::element_type>::epsilon() * 100;
    for (auto i = 0; i < k; ++i) {
      for (auto j = i; j < k; ++j) {
        const auto test = dot_product(B[i], B[j]);
        //        std::cout << "i= " << i << " j= " << j << " dot= " << test <<
        //        std::endl;
        if (i == j) {
          TA_ASSERT(test - 1.0 < tolerance);
        } else {
          TA_ASSERT(test < tolerance);
        }
      }
    }
#endif

    return E.segment(0, n_roots_);
  }

 private:
  unsigned int n_roots_;
  bool symmetric_;
  unsigned int n_guess_;
  unsigned int max_n_guess_;
  std::deque<value_type> eigen_vector_;
};

}  // namespace mpqc

#endif  // SRC_MPQC_MATH_LINALG_DAVIDSON_DIAG_H_
