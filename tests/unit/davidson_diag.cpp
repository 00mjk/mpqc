//
// Created by Chong Peng on 2/21/17.
//

#include "mpqc/math/linalg/davidson_diag.h"
#include "mpqc/math/tensor/clr/array_to_eigen.h"

#include "catch.hpp"

using namespace mpqc;

TEST_CASE("Symmetric Davidson Algorithm", "[symm-davidson]") {
  using Array = TA::DistArray<TA::TensorD, TA::DensePolicy>;

  // matrix size
  const auto n = 400;
  const auto sparse = 0.1;
  const auto n_roots = 3;
  const auto n_guess = 3;
  const auto converge = 1.0e-6;
  const auto max_iter = 20;

  // initialize matrix
  RowMatrix<double> A = RowMatrix<double>::Zero(n, n);
  for (auto i = 0; i < n; i++) {
    A(i, i) = i + 1;
  }
  A = A + sparse * RowMatrix<double>::Random(n, n);

  RowMatrix<double> A_T = A.transpose();
  A = 0.5*(A_T + A);
  // Warning!
  // A = 0.5*(A.transpose() + A) didn't work

  // eigen solve
  Eigen::SelfAdjointEigenSolver<RowMatrix<double>> es(A);
  auto e = es.eigenvalues().segment(0, n_roots);

//  std::cout << "Eigen result: " << std::endl << e << std::endl;

  TA::TiledRange1 tr_n{0, 100, 200, 300, n};
  TA::TiledRange1 tr_guess{0, 1};

  auto A_ta = array_ops::eigen_to_array<TA::TensorD, TA::DensePolicy>(
      TA::get_default_world(), A, tr_n, tr_n);

  // build guess vector
  RowMatrix<double> guess = RowMatrix<double>::Identity(n, n_guess);

  std::vector<Array> guess_ta(n_guess);

  for (auto i = 0; i < n_guess; i++) {
    guess_ta[i] = array_ops::eigen_to_array<TA::TensorD, TA::DensePolicy>(
        TA::get_default_world(), guess.col(i), tr_n, tr_guess);
  }

  DavidsonDiag<Array> dvd(n_roots, n_guess);

  EigenVector<double> diagonal = A.diagonal();

  auto pred = [&diagonal](const double& e, Array& guess) {

    auto task = [&diagonal, &e](TA::TensorD& result_tile) {
      const auto& range = result_tile.range();
      double norm = 0.0;
      for (const auto& i : range) {
        const auto result = result_tile[i] / (e - diagonal[i[0]]);
        result_tile[i] = result;
        norm += result * result;
      }
      return std::sqrt(norm);
    };

    TA::foreach_inplace(guess, task);
    guess.world().gop.fence();

  };

  EigenVector<double> eig = EigenVector<double>::Zero(n_roots);
  for (auto i = 0; i < max_iter; i++) {

//    std::cout << "Iter: " << i << std::endl;
    const auto n_v = guess_ta.size();
    std::vector<Array> HB(n_v);

    for (auto i = 0; i < n_v; i++) {
      HB[i]("i,j") = A_ta("i,k") * guess_ta[i]("k,j");
    }

    EigenVector<double> eig_new = dvd.extrapolate(HB, guess_ta, pred);

//    std::cout << eig_new << std::endl;
//    std::cout << "norm= " << (eig - eig_new).norm() << "\n";

    if ((eig - eig_new).norm() < converge) {
      break;
    }

    eig = eig_new;
  }

  CHECK((e - eig).norm() < converge);
}


TEST_CASE("Nonsymmetric Davidson Algorithm", "[nonsymm-davidson]") {
  using Array = TA::DistArray<TA::TensorD, TA::DensePolicy>;

  // matrix size
  const auto n = 100;
  const auto sparse = 0.01;
  const auto n_roots = 3;
  const auto n_guess = 3;
  const auto converge = 1.0e-7;
  const auto max_iter = 20;

  // initialize matrix
  RowMatrix<double> A = RowMatrix<double>::Zero(n, n);
  for (auto i = 0; i < n; i++) {
    A(i, i) = i + 1;
  }
  A = A + sparse * RowMatrix<double>::Random(n, n);

  // eigen solve
  Eigen::EigenSolver<RowMatrix<double>> es(A);
  EigenVector<double> e_all = es.eigenvalues().real();
  // sort
  std::sort(e_all.data(), e_all.data() + e_all.size());
  auto e = e_all.segment(0, n_roots);

  std::cout << "Eigen result: " << std::endl << e << std::endl;

  TA::TiledRange1 tr_n{0, 50, n};
  TA::TiledRange1 tr_guess{0, 1};

  auto A_ta = array_ops::eigen_to_array<TA::TensorD, TA::DensePolicy>(
      TA::get_default_world(), A, tr_n, tr_n);

  // build guess vector
  RowMatrix<double> guess = RowMatrix<double>::Identity(n, n_guess);

  std::vector<Array> guess_ta(n_guess);

  for (auto i = 0; i < n_guess; i++) {
    guess_ta[i] = array_ops::eigen_to_array<TA::TensorD, TA::DensePolicy>(
        TA::get_default_world(), guess.col(i), tr_n, tr_guess);
  }

  DavidsonDiag<Array> dvd(n_roots, n_guess, false);

  EigenVector<double> diagonal = A.diagonal();

  auto pred = [&diagonal](const double& e, Array& guess) {

    auto task = [&diagonal, &e](TA::TensorD& result_tile) {
      const auto& range = result_tile.range();
      double norm = 0.0;
      for (const auto& i : range) {
        const auto result = result_tile[i] / (e - diagonal[i[0]]);
        result_tile[i] = result;
        norm += result * result;
      }
      return std::sqrt(norm);
    };

    TA::foreach_inplace(guess, task);
    guess.world().gop.fence();

  };

  EigenVector<double> eig = EigenVector<double>::Zero(n_roots);
  for (auto i = 0; i < max_iter; i++) {

    std::cout << "Iter: " << i << std::endl;
    const auto n_v = guess_ta.size();
    std::vector<Array> HB(n_v);

    for (auto i = 0; i < n_v; i++) {
      HB[i]("i,j") = A_ta("i,k") * guess_ta[i]("k,j");
    }

    EigenVector<double> eig_new = dvd.extrapolate(HB, guess_ta, pred);

    std::cout << eig_new << std::endl;
    std::cout << "norm= " << (eig - eig_new).norm() << "\n";

    if ((eig - eig_new).norm() < converge) {
      break;
    }

    eig = eig_new;
  }

  CHECK((e - eig).norm() < converge);
}