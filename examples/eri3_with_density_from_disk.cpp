#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/array.hpp>

#include <memory>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <chrono>

#include "../include/tbb.h"
#include "../include/libint.h"
#include "../include/tiledarray.h"
#include "../include/btas.h"

#include "../utility/make_array.h"
#include "../utility/parallel_print.h"
#include "../utility/array_storage.h"
#include "../utility/time.h"
#include "../utility/ta_helpers.h"

#include "../tensor/conversions/tile_pimpl_to_ta_tensor.h"

#include "../molecule/atom.h"
#include "../molecule/cluster.h"
#include "../molecule/molecule.h"
#include "../molecule/clustering_functions.h"
#include "../molecule/make_clusters.h"

#include "../basis/atom_basisset.h"
#include "../basis/basis_set.h"
#include "../basis/cluster_shells.h"
#include "../basis/basis.h"

#include "../integrals/btas_to_ta_tensor.h"
#include "../integrals/btas_to_low_rank_tensor.h"
#include "../integrals/make_engine.h"
#include "../integrals/ta_tensor_to_low_rank_tensor.h"
#include "../integrals/integral_engine_pool.h"
#include "../integrals/sparse_task_integrals.h"
#include "../integrals/dense_task_integrals.h"

#include "../purification/sqrt_inv.h"
#include "../purification/purification_devel.h"

using namespace tcc;
namespace ints = integrals;

namespace boost {
namespace serialization {

template <typename Archive>
void serialize(Archive &ar, RowMatrixXd &m, const unsigned int version) {
    unsigned int rows = 0;
    unsigned int cols = 0;
    ar &rows;
    ar &cols;

    m.resize(rows, cols);

    ar &boost::serialization::make_array(m.data(), m.size());
}


} // serialization
} // boost

RowMatrixXd read_density_from_file(std::string const &file_name) {
    RowMatrixXd D;
    std::ifstream dfile(file_name.c_str());
    if (dfile.good()) {
        boost::archive::binary_iarchive ia(dfile, std::ios::binary);
        ia >> D;
        dfile.close();
    } else {
        throw;
    }
    return D;
}

int main(int argc, char **argv) {
    auto &world = madness::initialize(argc, argv);
    std::string mol_file = "";
    std::string basis_name = "";
    std::string df_basis_name = "";
    std::string density_file = "";
    int bs_nclusters = 0;
    int dfbs_nclusters = 0;
    double threshold = 1e-11;
    if (argc >= 7) {
        mol_file = argv[1];
        density_file = argv[2];
        basis_name = argv[3];
        df_basis_name = argv[4];
        bs_nclusters = std::stoi(argv[5]);
        dfbs_nclusters = std::stoi(argv[6]);
    } else if (argc > 9 || argc < 7) {
        std::cout << "input is $./program mol_file density_matrix_file "
                     "basis_file df_basis_file "
                     "bs_clusters dfbs_clusters low_rank_threshhold(optional) "
                     "debug(optional)\n";
        return 0;
    }
    auto low_rank_threshold = (argc == 8) ? std::stod(argv[6]) : 1e-7;
    volatile auto debug = (argc == 9) ? std::stoi(argv[7]) : 0;
    if (world.rank() == 0) {
        std::cout << "Mol file is " << mol_file << std::endl;
        std::cout << "D file is " << density_file << std::endl;
        std::cout << "basis is " << basis_name << std::endl;
        std::cout << "df basis is " << df_basis_name << std::endl;
        std::cout << "Using " << bs_nclusters << " bs clusters" << std::endl;
        std::cout << "Using " << dfbs_nclusters << " dfbs clusters"
                  << std::endl;
        std::cout << "low rank threshhold is " << low_rank_threshold
                  << std::endl;
    }

    TiledArray::SparseShape<float>::threshold(threshold);
    utility::print_par(world, "Sparse threshold is ",
                       TiledArray::SparseShape<float>::threshold(), "\n");

    auto mol = molecule::read_xyz(mol_file);

    auto bs_clusters = molecule::attach_hydrogens_kmeans(mol, bs_nclusters);
    auto dfbs_clusters = molecule::attach_hydrogens_kmeans(mol, bs_nclusters);

    basis::BasisSet bs{basis_name};
    basis::BasisSet df_bs{df_basis_name};

    std::streambuf *cout_sbuf = std::cout.rdbuf(); // Silence libint printing.
    std::ofstream fout("/dev/null");
    std::cout.rdbuf(fout.rdbuf());
    basis::Basis basis{bs.create_basis(bs_clusters)};
    basis::Basis df_basis{df_bs.create_basis(dfbs_clusters)};
    std::cout.rdbuf(cout_sbuf);

    auto tr1 = basis.create_flattend_trange1();
    auto tr = TA::TiledRange{tr1, tr1};

    RowMatrixXd D_eig;
    if (world.rank() == 0) {
        auto D_eig = read_density_from_file(density_file);
    }
    world.gop.fence();

    TA::Tensor<float> tile_norms(tr.tiles(), 0.0);
    for (auto i = 0; i < tile_norms.size(); ++i) {
        auto range = tr.make_tile_range(i);
        auto const &size = range.size();
        auto const &start = range.start();

        tile_norms[i]
            = D_eig.block(start[0], start[1], size[0], size[1]).lpNorm<2>();
    }

    TA::SparseShape<float> shape(world, tile_norms, tr);

    auto D_TA
        = TA::Array<double, 2, tensor::TilePimpl<double>, TA::SparsePolicy>(
            world, tr, shape);

    auto const &pmap = D_TA.get_pmap();
    auto beg = pmap->begin();
    auto end = pmap->end();

    if (world.rank() == 0) {
        for (auto i = 0; i < pmap->size(); ++i) {
            if (!D_TA.is_zero(i)) {
                auto range = tr.make_tile_range(i);
                auto const &start = range.start();
                auto const &size = range.size();
                RowMatrixXd mat
                    = D_eig.block(start[0], start[1], size[0], size[1]);

                RowMatrixXd L, R;
                if (algebra::Decompose_Matrix(mat, L, R, low_rank_threshold)) {
                    auto tile = tensor::TilePimpl<double>{range, 
                        tensor::TileVariant<double>{
                            tensor::FullRankTile<double>{mat}},
                        low_rank_threshold};
                    D_TA.set(i, tile);
                } else {
                    auto tile = tensor::TilePimpl<double>{range, 
                        tensor::TileVariant<double>{
                            tensor::LowRankTile<double>{L,R}},
                        low_rank_threshold};
                    D_TA.set(i, tile);
                }
            }
        }
    }

    utility::print_size_info(D_TA, "D");

    madness::finalize();
    return 0;
}
