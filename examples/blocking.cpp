#include <memory>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <rapidjson/document.h>

#include "../include/libint.h"
#include "../include/tiledarray.h"
#include "../include/btas.h"

#include "../utility/make_array.h"
#include "../utility/parallel_print.h"
#include "../utility/parallel_break_point.h"
#include "../utility/array_storage.h"
#include "../utility/time.h"
#include "../utility/json_input.h"

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
#include "../integrals/make_engine.h"
#include "../integrals/integral_engine_pool.h"
#include "../integrals/sparse_task_integrals.h"

#include "../scf/soad.h"
#include "../scf/diagonalize_for_coffs.hpp"
#include "../ta_routines/array_to_eigen.h"

using namespace tcc;
namespace ints = integrals;


// static auto direct_two_e_ao =
// std::make_shared<tcc::cc::TwoBodyIntGenerator<libint2::Coulomb>>();

static std::map<int, std::string> atom_names = {{1,  "H"},
                                                {2,  "He"},
                                                {3,  "Li"},
                                                {4,  "Be"},
                                                {5,  "B"},
                                                {6,  "C"},
                                                {7,  "N"},
                                                {8,  "O"},
                                                {9,  "F"},
                                                {10, "Ne"},
                                                {11, "Na"},
                                                {12, "Mg"}};

void
main_print_clusters(std::vector <std::shared_ptr<molecule::Cluster>> const &bs,
                    std::ostream &os) {
    std::vector <std::vector<molecule::Atom>> clusters;
    auto total_atoms = 0ul;
    for (auto const &cluster : bs) {
        std::vector <molecule::Atom> temp;
        for (auto atom : molecule::collapse_to_atoms(*cluster)) {
            temp.push_back(std::move(atom));
            ++total_atoms;
        }
        clusters.push_back(std::move(temp));
    }

    os << total_atoms << std::endl;
    os << "Whole molecule" << std::endl;

    for (auto const &cluster : clusters) {
        for (auto const &atom : cluster) {
            auto center = 0.52917721092 * atom.center();
            os << atom_names[atom.charge()] << " " << center[0] << " "
            << center[1] << " " << center[2] << std::endl;
        }
    }
    os << std::endl;
    auto counter = 0ul;
    for (auto const &cluster : clusters) {
        os << cluster.size() << std::endl;
        os << "Cluster " << counter++ << std::endl;
        for (auto const &atom : cluster) {
            auto center = 0.52917721092 * atom.center();
            os << atom_names[atom.charge()] << " " << center[0] << " "
            << center[1] << " " << center[2] << std::endl;
        }
        os << std::endl;
    }
}

int try_main(int argc, char *argv[], madness::World &world) {


    // parse the input
    rapidjson::Document in;
    parse_input(argc, argv, in);

    if (!in.HasMember("xyz file") || !in.HasMember("number of bs clusters")
        || !in.HasMember("number of dfbs clusters")
        || !in.HasMember("mo block size")) {
        if (world.rank() == 0) {
            std::cout << "At a minimum your input file must provide\n";
            std::cout << "\"xyz file\", which is path to an xyz input\n";
            std::cout << "\"number of bs clusters\", which is the number of "
                    "clusters in the obs\n";
            std::cout << "\"number of dfbs clusters\", which is the number of "
                    "clusters in the dfbs\n";
            std::cout << "\"mo block size\", which is the block size for MO "
                    "orbitals\n";
        }
    }
    // Get necessary info
    std::string mol_file = in["xyz file"].GetString();
    int bs_nclusters = in["number of bs clusters"].GetInt();
    int dfbs_nclusters = in["number of dfbs clusters"].GetInt();
    int nocc_clusters = in["number of occupied clusters"].GetInt();
    std::size_t blocksize = in["mo block size"].GetInt();


    // Get basis info
    std::string basis_name = in.HasMember("basis") ? in["basis"].GetString()
                                                   : "cc-pvdz";
    std::string df_basis_name = in.HasMember("df basis")
                                ? in["df basis"].GetString()
                                : "cc-pvdz-ri";

    // Get thresh info
    auto threshold = in.HasMember("block sparse threshold")
                     ? in["block sparse threshold"].GetDouble()
                     : 1e-13;
    auto low_rank_threshold = in.HasMember("low rank threshold")
                              ? in["low rank threshold"].GetDouble()
                              : 1e-8;

    // Get printing info
    bool print_clusters = in.HasMember("print clusters")
                          ? in["print clusters"].GetBool()
                          : false;

    // get other info
    bool frozen_core = in.HasMember("frozen core")
                       ? in["frozen core"].GetBool()
                       : false;

    volatile int debug
            = in.HasMember("debug break") ? in["debug break"].GetInt() : 0;

    utility::parallal_break_point(world, debug);

    if (world.rank() == 0) {
        std::cout << "Mol file is " << mol_file << std::endl;
        std::cout << "basis is " << basis_name << std::endl;
        std::cout << "df basis is " << df_basis_name << std::endl;
        std::cout << "Using " << bs_nclusters << " bs clusters"
        << std::endl;
        std::cout << "Using " << dfbs_nclusters << " dfbs clusters"
        << std::endl;
        std::cout << "low rank threshhold is " << low_rank_threshold
        << std::endl;
        if (print_clusters) {
            std::cout << "Printing clusters to clusters_bs.xyz and "
                    "cluster_dfbs.xyz." << std::endl;
        }
    }

    TiledArray::SparseShape<float>::threshold(threshold);
    utility::print_par(world, "Sparse threshold is ",
                       TiledArray::SparseShape<float>::threshold(), "\n");

    auto mol = molecule::read_xyz(mol_file);
    auto charge = 0;
    auto occupation = mol.occupation(charge);
    auto repulsion_energy = mol.nuclear_repulsion();
    auto core_electron = mol.core_electrons();

    utility::print_par(world, "Nuclear repulsion_energy = ",
                       repulsion_energy, "\n");

    auto bs_clusters = molecule::attach_hydrogens_kmeans(mol, bs_nclusters);
    auto dfbs_clusters
            = molecule::attach_hydrogens_kmeans(mol, dfbs_nclusters);

    world.gop.fence();
    if (world.rank() == 0) {
        if (print_clusters) {
            std::string obs_file = "clusters_bs.xyz";
            std::string dfbs_file = "clusters_dfbs.xyz";

            std::cout << "Printing clusters\n";
            std::cout
            << "\tobs clusters to " << obs_file
            << ": over ride with \"basis clusters file\" keyword\n";
            std::cout << "\tdfbs clusters to " << dfbs_file
            << ": over ride with \"df basis clusters file\" "
                    "keyword\n";

            std::ofstream bs_cluster_file(obs_file);
            main_print_clusters(bs_clusters, bs_cluster_file);
            bs_cluster_file.close();
            std::ofstream dfbs_cluster_file(dfbs_file);
            main_print_clusters(dfbs_clusters, dfbs_cluster_file);
            dfbs_cluster_file.close();
        }
    }
    world.gop.fence();

    basis::BasisSet bs{basis_name};
    basis::BasisSet df_bs{df_basis_name};


    std::streambuf *cout_sbuf
            = std::cout.rdbuf(); // Silence libint printing.
    std::ofstream fout("/dev/null");
    std::cout.rdbuf(fout.rdbuf());
    basis::Basis basis{bs.create_basis(bs_clusters)};
    basis::Basis df_basis{df_bs.create_basis(dfbs_clusters)};
    std::cout.rdbuf(cout_sbuf);

    if (world.rank() == 0) {
        std::cout << "Basis trange " << std::endl;
        TA::TiledRange1 bs_range = basis.create_flattend_trange1();
        std::cout << bs_range << std::endl;
        auto iter = bs_range.begin();
        for (; iter != bs_range.end() - 1; ++iter){
            std::cout << iter->first << " ";
        }
        std::cout << iter->first << " ";
        std::cout << iter->second << std::endl;
        TA::TiledRange1 dfbs_range = df_basis.create_flattend_trange1();
        std::cout << "DF Basis trange " << std::endl;
        std::cout << dfbs_range << std::endl;
    }

    libint2::init();

    // Make a btas to decomposed tensor function
    struct convert_2d {
        double cut_;

        convert_2d(double thresh) : cut_{thresh} { }

        using TileType = tensor::Tile<tensor::DecomposedTensor<double>>;

        TileType operator()(tensor::ShallowTensor<2> const &bt) {
            auto range = bt.range();

            auto const extent = range.extent();
            const auto i = extent[0];
            const auto j = extent[1];
            auto local_range = TA::Range{i, j};

            auto tensor = TA::Tensor<double>(local_range);
            const auto b_data = bt.tensor().data();
            const auto size = bt.tensor().size();
            std::copy(b_data, b_data + size, tensor.data());

            auto dense
                    = tensor::DecomposedTensor<double>(cut_,
                                                       std::move(tensor));

            return tensor::Tile<tensor::DecomposedTensor<double>>(
                    range, std::move(dense));
        }
    };

    auto bs_basis_array = utility::make_array(basis, basis);

    // Compute overlap.
    auto overlap_pool = ints::make_pool(ints::make_1body("overlap", basis));
    auto S = BlockSparseIntegrals(world, overlap_pool, bs_basis_array,
                                  convert_2d(low_rank_threshold));

    // Overlap Range
    if (world.rank() == 0) {
        std::cout << "Overlap trange " << std::endl;
        TA::TiledRange1 overlap_range = S.trange().data().front();
        std::cout << overlap_range << std::endl;
    }
//
//    auto to_ta =
//            [](tensor::Tile<tensor::DecomposedTensor<double>> const &t) {
//                auto tensor = tensor::algebra::combine(t.tile());
//                auto range = t.range();
//                return TA::Tensor<double>(range, tensor.data());
//            };
//    auto S_TA = TA::to_new_tile_type(S, to_ta);
//
//    // Compute T
//    auto kinetic_pool = ints::make_pool(ints::make_1body("kinetic", basis));
//    auto T = BlockSparseIntegrals(world, kinetic_pool, bs_basis_array,
//                                  convert_2d(low_rank_threshold));
//
//    /* // Compute V */
//    auto nuclear_pool = ints::make_pool(ints::make_1body("nuclear", basis));
//    auto V = BlockSparseIntegrals(world, nuclear_pool, bs_basis_array,
//                                  convert_2d(low_rank_threshold));
//
//    /* // Compute Hcore */
//    utility::print_par(world, "Computing Hcore\n");
//    decltype(V) H;
//    H("i,j") = T("i,j") + V("i,j");
//    world.gop.fence();
//
//    auto H_TA = TA::to_new_tile_type(H, to_ta);
//
//    auto to_decomp = [=](TA::Tensor<double> const &t) {
//        auto range = t.range();
//
//        auto const extent = range.extent();
//        const auto i = extent[0];
//        const auto j = extent[1];
//        auto local_range = TA::Range{i, j};
//
//        auto tensor = TA::Tensor<double>(local_range, t.data());
//        auto dense = tensor::DecomposedTensor<double>(low_rank_threshold,
//                                                      std::move(tensor));
//
//        return tensor::Tile<tensor::DecomposedTensor<double>>(
//                range, std::move(dense));
//    };
//
//    /* // Begin Two electron integrals section. */
//    auto eri_pool = ints::make_pool(ints::make_2body(basis, df_basis));
//
//    auto to_decomp_with_decompose = [=](TA::Tensor<double> const &t) {
//        auto range = t.range();
//
//        auto const extent = range.extent();
//        const auto i = extent[0];
//        const auto j = extent[1];
//        auto local_range = TA::Range{i, j};
//
//        auto tensor = TA::Tensor<double>(local_range, t.data());
//        auto dense = tensor::DecomposedTensor<double>(low_rank_threshold,
//                                                      std::move(tensor));
//
//        auto test = tensor::algebra::two_way_decomposition(dense);
//        if (!test.empty()) {
//            dense = std::move(test);
//        }
//
//        return tensor::Tile<tensor::DecomposedTensor<double>>(
//                range, std::move(dense));
//    };
//    /* // Computing Eri2 */
//    utility::print_par(world, "Starting 2 Center Integrals\n");
//    TA::Array<double, 2, tensor::Tile<tensor::DecomposedTensor<double>>,
//            TA::SparsePolicy> V_inv_oh;
//    {
//        utility::print_par(world, "\tStarting V\n");
//        auto eri2 = ints::BlockSparseIntegrals(
//                world, eri_pool, utility::make_array(df_basis, df_basis),
//                integrals::compute_functors::BtasToTaTensor{});
//
//        decltype(eri2) L_inv_TA;
//        {
//            utility::print_par(world, "\tReplicating to Eigen\n");
//            auto eig_E2 = array_ops::array_to_eigen(eri2);
//            Eig::LLT<decltype(eig_E2)> llt(eig_E2);
//            eig_E2 = llt.matrixL();
//            decltype(eig_E2) eig_L_inv = eig_E2.inverse();
//            utility::print_par(world, "\tConverting back to TA\n");
//            L_inv_TA = array_ops::eigen_to_array<TA::Tensor < double>>
//            (
//                    world, eig_L_inv, eri2.trange().data()[0],
//                            eri2.trange().data()[1]);
//        }
//
//        utility::print_par(world, "\tDecomposing Tiles\n");
//        V_inv_oh = TA::to_new_tile_type(L_inv_TA, to_decomp_with_decompose);
//        utility::print_size_info(V_inv_oh, "V^{-1/2}");
//    }
//    decltype(V_inv_oh)::wait_for_lazy_cleanup(world, 2);
//
//
//    struct convert_3d {
//        double cut_;
//
//        convert_3d(double thresh) : cut_{thresh} { }
//
//        using TileType = tensor::Tile<tensor::DecomposedTensor<double>>;
//
//        TileType operator()(tensor::ShallowTensor<3> const &bt) {
//            auto range = bt.range();
//
//            auto const extent = range.extent();
//            const auto X = extent[0];
//            const auto i = extent[1];
//            const auto j = extent[2];
//            auto local_range = TA::Range{X, i, j};
//
//            auto tensor = TA::Tensor<double>(local_range);
//            const auto b_data = bt.tensor().data();
//            const auto size = bt.tensor().size();
//            std::copy(b_data, b_data + size, tensor.data());
//
//            auto dense
//                    = tensor::DecomposedTensor<double>(cut_,
//                                                       std::move(tensor));
//
//            auto test = tensor::algebra::two_way_decomposition(dense);
//            if (!test.empty()) {
//                dense = std::move(test);
//            }
//
//            return tensor::Tile<tensor::DecomposedTensor<double>>(
//                    range, std::move(dense));
//        }
//    };
//
//    // Compute center integrals
//    utility::print_par(world, "\nStarting 3 Center Integrals\n");
//    auto E0 = tcc_time::now();
//    auto Xab = ints::BlockSparseIntegrals(
//            world, eri_pool, utility::make_array(df_basis, basis, basis),
//            convert_3d(low_rank_threshold));
//    auto E1 = tcc_time::now();
//    auto etime = tcc_time::duration_in_s(E0, E1);
//    utility::print_par(world, "Time to compute 3 center integrals ", etime,
//                       " s\n");
//    utility::print_size_info(Xab, "E");
//    decltype(Xab)::wait_for_lazy_cleanup(world, 60);


    world.gop.fence();
    libint2::cleanup();
    return 0;
}


int main(int argc, char *argv[]) {

    int rc = 0;

    auto &world = madness::initialize(argc, argv);

    try {

        try_main(argc, argv, world);

    } catch (TiledArray::Exception &e) {
        std::cerr << "!! TiledArray exception: " << e.what() << "\n";
        rc = 1;
    } catch (madness::MadnessException &e) {
        std::cerr << "!! MADNESS exception: " << e.what() << "\n";
        rc = 1;
    } catch (SafeMPI::Exception &e) {
        std::cerr << "!! SafeMPI exception: " << e.what() << "\n";
        rc = 1;
    } catch (std::exception &e) {
        std::cerr << "!! std exception: " << e.what() << "\n";
        rc = 1;
    } catch (...) {
        std::cerr << "!! exception: unknown exception\n";
        rc = 1;
    }


    madness::finalize();
    return rc;
}