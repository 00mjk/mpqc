//
// task_integrals.h
//
// Copyright (C) 2014 Drew Lewis
// Maintainer Drew Lewis
//
// Based on file task_integrals.hpp from mpqc
//

#ifndef TCC_INTEGRALS_TASKINTEGRALS_H
#define TCC_INTEGRALS_TASKINTEGRALS_H

#include "../include/tiledarray.h"
#include "../include/btas.h"
#include "../include/libint2.h"
#include "../include/tbb.h"

#include "../basis/cluster_shells.h"
#include "integral_engine_pool.h"

#include "../basis/basis.h"

namespace tcc {
namespace integrals {

namespace detail {
class BtasTileFunctor {
  public:
    using TileType = TiledArray::Tensor<double>;

    template <typename It, typename SharedEnginePool>
    TileType operator()(It it, basis::Basis const *basis,
                        SharedEnginePool engines) const {
        auto range = it.make_range();

        TileType tile{range};
        for (auto i = 0ul; i < tile.range().volume(); ++i) {
            *(tile.data() + i) = i;
        }

        auto idx = it.index();
        std::vector<basis::ClusterShells> clusters;
        clusters.reserve(idx.size());
        for (auto const &i : idx) {
            // basis.cluster_shells returns a ref, which we are going to copy,
            // This may or may not be a big deal but I wanted to leave a note.
            clusters.push_back(basis->cluster_shells()[i]);
        }

        auto btas_tensor = compute_integrals(engines->local(), clusters);
        std::cout << "Btas_tensor = \n " << btas_tensor << std::endl;
        for (auto i = 0ul; i < 4; ++i) {
            for (auto j = 0ul; j < 4; ++j) {
                std::cout << "Slice " << i + 1 << " " << j + 1 << std::endl;
                for (auto k = 0ul; k < 4; ++k) {
                    for (auto l = 0ul; l < 4; ++l) {
                        std::cout << btas_tensor(i, j, k, l) << " ";
                    }
                    std::cout << std::endl;
                }
                std::cout << std::endl;
            }
        }

        return tile;
    }

  private:
    template <typename Engine>
    btas::Tensor<double>
    compute_integrals(Engine &engine,
                      std::vector<basis::ClusterShells> const &clusters) const {
        constexpr auto dims = pool_order<Engine>();

        std::vector<unsigned long> dim_sizes;
        dim_sizes.reserve(dims);
        for (auto i = 0ul; i < dims; ++i) {
            dim_sizes.push_back(clusters[i].flattened_nfunctions());
        }

        auto tensor = make_btas_tensor<dims>(dim_sizes);

        auto sh1_func = 0;
        for (auto const &sh1 : clusters[0].flattened_shells()) {
            auto nprim1 = sh1.nprim();
            auto sh2_func = 0;
            for (auto const &sh2 : clusters[1].flattened_shells()) {
                auto nprim2 = sh2.nprim();
                auto sh3_func = 0;
                for (auto const &sh3 : clusters[2].flattened_shells()) {
                    auto nprim3 = sh3.nprim();
                    auto sh4_func = 0;
                    for (auto const &sh4 : clusters[3].flattened_shells()) {
                        auto nprim4 = sh4.nprim();

                        std::cout << "lower bound = " << sh1_func << " "
                                  << sh2_func << " " << sh3_func << " "
                                  << sh4_func << std::endl;

                        std::cout << "upper bound = " << sh1_func + nprim1
                                  << " " << sh2_func + nprim2 << " "
                                  << sh3_func + nprim3 << " "
                                  << sh4_func + nprim4 << std::endl;
                        std::cout << std::endl;

                        auto lower_bound
                            = {sh1_func, sh2_func, sh3_func, sh4_func};
                        auto upper_bound
                            = {sh1_func + nprim1, sh2_func + nprim2,
                               sh3_func + nprim3, sh4_func + nprim4};

                        auto view = btas::make_view(
                            tensor.range().slice(lower_bound, upper_bound),
                            tensor.storage());
                        
                        std::cout << "View size = " << view.size() << std::endl;

                        auto buf = engine.compute(sh1, sh2, sh3, sh4);
                        auto ijkl = 0ul;
                        for (auto i = 0ul; i < nprim1; ++i) {
                            for (auto j = 0ul; j < nprim2; ++j) {
                                std::cout << "buf Slice " << i + 1 << " " << j + 1 << std::endl;
                                for (auto k = 0ul; k < nprim3; ++k) {
                                    for (auto l = 0ul; l < nprim4; ++l) {
                                        view(i, j, k, l) = buf[ijkl];
                                        std::cout << buf[ijkl] << " ";
                                    }
                                    std::cout << std::endl;
                                }
                                std::cout << std::endl;
                            }
                        }

                        sh4_func = nprim4;
                    }
                    sh3_func = nprim3;
                }
                sh2_func = nprim2;
            }
            sh1_func = nprim1;
        }

        return tensor;
    }

    template <unsigned long N,
              typename Integral> // requires is_integral<Integral>
    btas::Tensor<double>
    make_btas_tensor(std::vector<Integral> &sizes) const {
        return btas_vector_init<N>(sizes);
    }

    template <unsigned long...>
    struct seq {};

    template <unsigned long N, unsigned long... S>
    struct gens : gens<N - 1, N - 1, S...> {};

    template <unsigned long... S>
    struct gens<0, S...> {
        typedef seq<S...> type;
    };

    template <typename Integral, unsigned long N, unsigned long... I>
    btas::Tensor<double>
    create_tensor(std::array<Integral, N> sizes, seq<I...>) const {
        return btas::Tensor<double>{std::get<I>(sizes)...};
    }

    template <unsigned long N, typename Integral>
    btas::Tensor<double>
    btas_vector_init(std::vector<Integral> const &vec) const {
        return create_tensor(vector_to_array<N>(vec), typename gens<N>::type{});
    }

    template <unsigned long N, typename T>
    std::array<T, N> // Should problably check vec length.
        vector_to_array(std::vector<T> const &vec) const {
        std::array<unsigned long, N> a;
        for (auto i = 0ul; i < N; ++i) {
            a[i] = vec[i];
        }
        return a;
    }

}; // class BtasTileFunctor
} // namespace detail

template <typename Array, typename SharedEnginePool, typename TileFunctor>
void initialize_tiles(Array &a, SharedEnginePool engines,
                      basis::Basis const &basis, TileFunctor tf) {
    const auto end = a.end();
    for (auto it = a.begin(); it != end; ++it) {
        auto call_tf = [=](basis::Basis const *basis_) {
            *it = (tf(it, basis_, engines));
        };
        a.get_world().taskq.add(call_tf, &basis);
    }
}

template <unsigned long order, typename TileType>
TiledArray::Array<double, order, TileType>
create_array(madness::World &world, basis::Basis const &basis) {

    std::vector<TiledArray::TiledRange1> trange1_collector;
    trange1_collector.reserve(order);

    const auto trange1 = basis.create_trange1();
    for (auto i = 0ul; i < order; ++i) {
        trange1_collector.push_back(trange1);
    }

    TiledArray::TiledRange trange(trange1_collector.begin(),
                                  trange1_collector.end());

    return TiledArray::Array<double, order, TileType>(world, trange);
}

template <typename SharedEnginePool,
          typename TileFunctor = typename detail::BtasTileFunctor>
TiledArray::Array<double, integrals::pool_order<SharedEnginePool>(),
                  typename TileFunctor::TileType>
Integrals(madness::World &world, SharedEnginePool engines,
          basis::Basis const &basis, TileFunctor tf = TileFunctor{}) {

    constexpr auto tensor_order = integrals::pool_order<decltype(engines)>();
    using TileType = typename TileFunctor::TileType;

    auto array = create_array<tensor_order, TileType>(world, basis);

    initialize_tiles(array, std::move(engines), basis, tf);
    return array;
}

} // namespace integrals
} // namespace tcc


#endif // TCC_INTEGRALS_TASKINTEGRALS_H
