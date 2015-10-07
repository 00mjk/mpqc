#pragma once

#ifndef MPQC_INTEGRALS_TASKINTEGRALSCOMMON_H
#define MPQC_INTEGRALS_TASKINTEGRALSCOMMON_H

#include "task_integrals_helper.h"
#include "../common/typedefs.h"
#include "../include/tiledarray.h"
// #include "../include/tbb.h"
#include "../basis/basis.h"

#include <memory>
#include <array>
#include <vector>
#include <utility>

namespace mpqc {
namespace integrals {

template <typename E>
using ShrPool = std::shared_ptr<Epool<E>>;

template <unsigned long N>
using Barray = std::array<basis::Basis, N>;

template <typename T>
using OrdTileVec = std::vector<std::pair<unsigned long, T>>;

const static Shell unit_shell = Shell::unit();

namespace detail {

template <unsigned long N>
using ShrBases = std::shared_ptr<Barray<N>>;

template <typename Op>
using Ttype = decltype(std::declval<Op>()(std::declval<TA::TensorD>()));

// Create TRange from bases
template <unsigned long N>
TRange create_trange(Barray<N> const &basis_array) {

    std::vector<TRange1> trange1s;
    trange1s.reserve(N);

    for (auto i = 0ul; i < N; ++i) {
        trange1s.emplace_back(basis_array[i].create_trange1());
    }

    return TRange(trange1s.begin(), trange1s.end());
}

template<typename Tile, typename Array>
void set_array(std::vector<std::pair<unsigned long, Tile>> &tiles, Array &a){
    for (auto &&tile : tiles) {
        const auto ord = tile.first;
        if (!a.is_zero(ord)) {
            assert(!tile.second.empty());
            a.set(ord, std::move(tile.second));
        }
    }
}

} // namespace detail

} // namespace integrals
} // namespace mpqc

#endif //  MPQC_INTEGRALS_TASKINTEGRALSCOMMON_H
