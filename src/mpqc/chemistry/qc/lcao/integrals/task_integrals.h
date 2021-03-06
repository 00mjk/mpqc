//
// task_integrals.h
//
// Copyright (C) 2015 Drew Lewis
// Maintainer Drew Lewis
//
// Based on file task_integrals.hpp from mpqc
//

#ifndef MPQC4_SRC_MPQC_CHEMISTRY_QC_INTEGRALS_TASK_INTEGRALS_H_
#define MPQC4_SRC_MPQC_CHEMISTRY_QC_INTEGRALS_TASK_INTEGRALS_H_

#include <TiledArray/tensor/tensor_map.h>
#include <TiledArray/tile_op/noop.h>

#include "mpqc/chemistry/qc/lcao/integrals/integral_builder.h"
#include "mpqc/chemistry/qc/lcao/integrals/screening/screen_base.h"
#include "mpqc/chemistry/qc/lcao/integrals/task_integrals_common.h"
#include "mpqc/util/misc/assert.h"

namespace mpqc {
namespace lcao {
namespace gaussian {

/*! \brief Construct sparse integral tensors from sets in parallel.
 *
 * This is needed for integrals such as the dipole integrals that come as a
 * set.
 *
 * \param shr_pool should be a std::shared_ptr to an IntegralTSPool
 * \param bases should be a std::array of Basis, which will be copied.
 * \param op needs to be a function or functor that takes a TA::TensorD && and
 * returns any valid tile type. Op is copied so it can be moved.
 * ```
 * auto t = [](TA::TensorD &&ten){return std::move(ten);};
 * ```
 */
template <typename E, typename Tile = TA::TensorD>
std::vector<TA::DistArray<Tile, TA::SparsePolicy>> sparse_xyz_integrals(
    madness::World &world, ShrPool<E> shr_pool, BasisArray<2> const &bases,
    std::function<Tile(TA::TensorD &&)> op = TA::detail::Noop<Tile,TA::TensorD, true>()) {
  // Build the Trange and Shape Tensor
  auto trange = detail::create_trange(bases);
  const auto tiles_range = trange.tiles_range();
  const auto tvolume = tiles_range.volume();

  using TileVec = std::vector<std::pair<unsigned long, Tile>>;
  std::array<TileVec, 3> tiles{
      {TileVec(tvolume), TileVec(tvolume), TileVec(tvolume)}};

  using NormVec = std::array<TA::TensorF, 3>;
  NormVec tile_norms{{TA::TensorF(tiles_range, 0.0),
                      TA::TensorF(tiles_range, 0.0),
                      TA::TensorF(tiles_range, 0.0)}};

  // Capture by ref since we are going to fence after loops.
  auto task_f = [&](int64_t ord, detail::IdxVec idx, TA::Range rng) {
    auto &eng = shr_pool->local();

    const auto idx0 = idx[0];
    const auto idx1 = idx[1];

    const auto t_volume = rng.volume();

    auto const &lobound = rng.lobound();
    std::array<std::size_t, 2> lb = {{lobound[0], lobound[1]}};
    std::array<std::size_t, 2> ub = {{lobound[0], lobound[1]}};

    std::vector<TA::TensorD> t_xyz = {
        TA::TensorD(rng, 0.0), TA::TensorD(rng, 0.0), TA::TensorD(rng, 0.0)};

    double dummy = 0.0;
    auto map = TA::make_const_map(&dummy, {0, 0}, {1, 1});

    auto const &shells0 = bases[0].cluster_shells()[idx0];
    auto const &shells1 = bases[1].cluster_shells()[idx1];

    for (auto const &s0 : shells0) {
      const auto n0 = s0.size();
      ub[0] += n0;

      lb[1] = ub[1] = lobound[1];
      for (auto const &s1 : shells1) {
        const auto n1 = s1.size();
        ub[1] += n1;

        const auto &bufs = eng.compute(s0, s1);
        MPQC_ASSERT(bufs.size() >= 4);

        if (bufs[0] != nullptr) {
          for (auto i = 1; i < 4; ++i) {
            TA::remap(map, bufs[i], lb, ub);
            t_xyz[i - 1].block(lb, ub) = map;
          }
        }

        lb[1] = ub[1];
      }
      lb[0] = ub[0];
    }

    std::array<double, 3> norm = {
        {t_xyz[0].norm(), t_xyz[1].norm(), t_xyz[2].norm()}};

    for (auto i = 0; i < 3; ++i) {
      const float norm_float = norm[i];
      tile_norms[i][ord] = norm_float;
      if (TA::SparseShape<float>::threshold() * t_volume <= norm_float) {
        tiles[i][ord].second = op(std::move(t_xyz[i]));
      }
    }
  };

  auto pmap = TA::SparsePolicy::default_pmap(world, tvolume);
  for (auto const ord : *pmap) {
    tiles[0][ord].first = ord;
    tiles[1][ord].first = ord;
    tiles[2][ord].first = ord;
    detail::IdxVec idx = tiles_range.idx(ord);
    world.taskq.add(task_f, ord, idx, trange.make_tile_range(ord));
  }
  world.gop.fence();

  std::vector<TA::DistArray<Tile, TA::SparsePolicy>> arrays(3);

  for (auto i = 0; i < 3; ++i) {
    TA::SparseShape<float> shape(world, tile_norms[i], trange);
    arrays[i] =
        TA::DistArray<Tile, TA::SparsePolicy>(world, trange, shape, pmap);
    detail::set_array(tiles[i], arrays[i]);
  }
  world.gop.fence();

  return arrays;
}

/*! \brief Construct dense integral tensors from sets in parallel.
 *
 * This is needed for integrals such as the dipole integrals that come as a
 * set.
 *
 * \param shr_pool should be a std::shared_ptr to an IntegralEnginePool
 * \param bases should be a std::array of Basis, which will be copied.
 * \param op needs to be a function or functor that takes a TA::TensorD && and
 * returns any valid tile type. Op is copied so it can be moved.
 * ```
 * auto t = [](TA::TensorD &&ten){return std::move(ten);};
 * ```
 */
template <typename E, typename Tile = TA::TensorD>
std::vector<TA::DistArray<Tile, TA::DensePolicy>> dense_xyz_integrals(
    madness::World &world, ShrPool<E> shr_pool, BasisArray<2> const &bases,
    std::function<Tile(TA::TensorD &&)> op = TA::detail::Noop<Tile,TA::TensorD, true>()) {
  // Build the Trange and Shape Tensor
  auto trange = detail::create_trange(bases);
  const auto tiles_range = trange.tiles_range();
  const auto tvolume = tiles_range.volume();

  using TileVec = std::vector<std::pair<unsigned long, Tile>>;
  std::array<TileVec, 3> tiles{
      {TileVec(tvolume), TileVec(tvolume), TileVec(tvolume)}};

  // Capture by ref since we are going to fence after loops.
  auto task_f = [&](int64_t ord, detail::IdxVec idx, TA::Range rng) {
    auto &eng = shr_pool->local();

    const auto idx0 = idx[0];
    const auto idx1 = idx[1];

    auto const &lobound = rng.lobound();
    std::array<std::size_t, 2> lb = {{lobound[0], lobound[1]}};
    std::array<std::size_t, 2> ub = {{lobound[0], lobound[1]}};

    std::vector<TA::TensorD> t_xyz = {
        TA::TensorD(rng, 0.0), TA::TensorD(rng, 0.0), TA::TensorD(rng, 0.0)};

    double dummy = 0.0;
    auto map = TA::make_const_map(&dummy, {0, 0}, {1, 1});

    auto const &shells0 = bases[0].cluster_shells()[idx0];
    auto const &shells1 = bases[1].cluster_shells()[idx1];

    for (auto const &s0 : shells0) {
      const auto n0 = s0.size();
      ub[0] += n0;

      lb[1] = ub[1] = lobound[1];
      for (auto const &s1 : shells1) {
        const auto n1 = s1.size();
        ub[1] += n1;

        const auto &bufs = eng.compute(s0, s1);
        MPQC_ASSERT(bufs.size() >= 4);

        if (bufs[0] != nullptr) {
          for (auto i = 1; i < 4; ++i) {
            TA::remap(map, bufs[i], lb, ub);
            t_xyz[i - 1].block(lb, ub) = map;
          }
        }

        lb[1] = ub[1];
      }
      lb[0] = ub[0];
    }

    for (auto i = 0; i < 3; ++i) {
      tiles[i][ord].second = op(std::move(t_xyz[i]));
    }
  };

  auto pmap = TA::SparsePolicy::default_pmap(world, tvolume);
  for (auto const ord : *pmap) {
    tiles[0][ord].first = ord;
    tiles[1][ord].first = ord;
    tiles[2][ord].first = ord;
    detail::IdxVec idx = tiles_range.idx(ord);
    world.taskq.add(task_f, ord, idx, trange.make_tile_range(ord));
  }
  world.gop.fence();

  std::vector<TA::DistArray<Tile, TA::DensePolicy>> arrays(3);

  for (auto i = 0; i < 3; ++i) {
    arrays[i] = TA::DistArray<Tile, TA::DensePolicy>(world, trange, pmap);
    detail::set_array(tiles[i], arrays[i]);
  }
  world.gop.fence();

  return arrays;
}

/*
 * interface to sparse_xyz_integral and dense_xyz_integral
 */

template <typename Tile, typename Policy, typename E>
std::vector<TA::DistArray<
    Tile, typename std::enable_if<std::is_same<Policy, TA::DensePolicy>::value,
                                  TA::DensePolicy>::type>>
xyz_integrals(
    madness::World &world, ShrPool<E> shr_pool, BasisArray<2> const &bases,
    std::function<Tile(TA::TensorD &&)> op = TA::detail::Noop<Tile,TA::TensorD, true>()) {
  return dense_xyz_integrals(world, shr_pool, bases, op);
};

template <typename Tile, typename Policy, typename E>
std::vector<TA::DistArray<
    Tile, typename std::enable_if<std::is_same<Policy, TA::SparsePolicy>::value,
                                  TA::SparsePolicy>::type>>
xyz_integrals(
    madness::World &world, ShrPool<E> shr_pool, BasisArray<2> const &bases,
    std::function<Tile(TA::TensorD &&)> op = TA::detail::Noop<Tile,TA::TensorD, true>()) {
  return sparse_xyz_integrals(world, shr_pool, bases, op);
};

/*! \brief Construct sparse integral tensors in parallel.
 *
 * \param shr_pool should be a std::shared_ptr to an IntegralTSPool
 * \param bases should be a std::array of Basis, which will be copied.
 * \param op needs to be a function or functor that takes a TA::TensorD && and
 * returns any valid tile type. Op is copied so it can be moved.
 * ```
 * auto t = [](TA::TensorD &&ten){return std::move(ten);};
 * ```
 *
 * \param screen should be a std::shared_ptr to a Screener.
 */
template <typename Tile = TA::TensorD, typename E>
TA::DistArray<Tile, TA::SparsePolicy> sparse_integrals(
    madness::World &world, ShrPool<E> shr_pool, BasisVector const &bases,
    std::shared_ptr<Screener> screen = std::make_shared<Screener>(Screener{}),
    std::function<Tile(TA::TensorD &&)> op = TA::detail::Noop<Tile,TA::TensorD, true>()) {
  // Build the Trange and Shape Tensor
  auto trange = detail::create_trange(bases);
  const auto tvolume = trange.tiles_range().volume();
  std::vector<std::pair<unsigned long, Tile>> tiles(tvolume);
  TA::TensorF tile_norms(trange.tiles_range(), 0.0);

  // Copy the Bases for the Integral Builder
  auto shr_bases = std::make_shared<BasisVector>(bases);

  // Make a pointer to an Integral builder.  Doing this because we want to use
  // it in Tasks.
  auto builder_ptr =
      make_integral_builder(std::move(shr_pool), std::move(shr_bases),
                            std::move(screen), std::move(op));

  auto task_f = [=](int64_t ord, detail::IdxVec idx_task, TA::Range rng,
                    TA::TensorF *tile_norms_ptr, Tile *out_tile) {

    // This is why builder was made into a shared_ptr.
    auto &builder = *builder_ptr;
    auto ta_tile = builder.integrals(idx_task, std::move(rng));

    const auto tile_volume = ta_tile.range().volume();
    const auto tile_norm = ta_tile.norm();

    // Keep tile if it was significant.
    bool save_norm =
        tile_norm >= tile_volume * TA::SparseShape<float>::threshold();
    if (save_norm) {
      *out_tile = builder.op(std::move(ta_tile));

      auto &norms = *tile_norms_ptr;
      norms[ord] = tile_norm;
    }
  };

  auto pmap = TA::SparsePolicy::default_pmap(world, tvolume);
  for (auto const ord : *pmap) {
    tiles[ord].first = ord;
    detail::IdxVec idx = trange.tiles_range().idx(ord);
    world.taskq.add(task_f, ord, idx, trange.make_tile_range(ord), &tile_norms,
                    &tiles[ord].second);
  }
  world.gop.fence();

  TA::SparseShape<float> shape(world, tile_norms, trange);
  TA::DistArray<Tile, TA::SparsePolicy> out(world, trange, shape, pmap);

  detail::set_array(tiles, out);
  world.gop.fence();
  out.truncate();
  world.gop.fence();

  return out;
}

/*! \brief Construct a dense integral tensor in parallel.
 *
 */
template <typename Tile = TA::TensorD, typename E>
TA::DistArray<Tile, TA::DensePolicy> dense_integrals(
    madness::World &world, ShrPool<E> shr_pool, BasisVector const &bases,
    std::shared_ptr<Screener> screen = std::make_shared<Screener>(Screener{}),
    std::function<Tile(TA::TensorD &&)> op = TA::detail::Noop<Tile,TA::TensorD, true>()) {
  TA::DistArray<Tile, TA::DensePolicy> out(world, detail::create_trange(bases));

  // Copy the Bases for the Integral Builder
  auto shr_bases = std::make_shared<BasisVector>(bases);

  // Make a pointer to a builder which can be shared by different tasks.
  auto builder_ptr =
      make_integral_builder(std::move(shr_pool), std::move(shr_bases),
                            std::move(screen), std::move(op));

  // builder is shared_ptr so just capture it by copy.
  auto task_func = [=](detail::IdxVec const &idx, TA::Range rng) {
    return builder_ptr->operator()(idx, std::move(rng));
  };

  auto const &trange = out.trange();
  auto const &pmap = *(out.pmap());
  for (auto const ord : pmap) {
    detail::IdxVec idx = trange.tiles_range().idx(ord);

    auto range = trange.make_tile_range(ord);
    madness::Future<Tile> tile = world.taskq.add(task_func, idx, range);

    out.set(ord, tile);
  }
  world.gop.fence();

  return out;
}

}  // namespace gaussian
}  // namespace lcao
}  // namespace mpqc

#endif  // MPQC4_SRC_MPQC_CHEMISTRY_QC_INTEGRALS_TASK_INTEGRALS_H_
