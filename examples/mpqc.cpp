#include <memory>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <libint2.hpp>

#include "../include/tiledarray.h"

#include "../utility/make_array.h"
#include "../utility/parallel_print.h"
#include "../utility/parallel_break_point.h"

#include "../utility/array_info.h"
#include "../utility/print_size_info.h"

#include "../utility/time.h"
#include "../utility/json_handling.h"
#include "../utility/parallel_file.h"

#include <mpqc/chemistry/molecule/atom.h>
#include <mpqc/chemistry/molecule/cluster.h>
#include <mpqc/chemistry/molecule/molecule.h>
#include <mpqc/chemistry/molecule/clustering_functions.h>
#include <mpqc/chemistry/molecule/make_clusters.h>

#include <mpqc/chemistry/qc/basis/atom_basisset.h>
#include <mpqc/chemistry/qc/basis/basis_set.h>
#include <mpqc/chemistry/qc/basis/cluster_shells.h>
#include <mpqc/chemistry/qc/basis/basis.h>

#include <mpqc/chemistry/qc/scf/diagonalize_for_coffs.hpp>
#include <mpqc/chemistry/qc/scf/scf.h>
#include <mpqc/chemistry/qc/scf/traditional_df_fock_builder.h>
#include <mpqc/chemistry/qc/scf/purification_density_build.h>
#include <mpqc/chemistry/qc/scf/eigen_solve_density_builder.h>
#include <mpqc/chemistry/qc/scf/traditional_four_center_fock_builder.h>
#include <mpqc/chemistry/qc/scf/mo_build.h>

#include <mpqc/chemistry/qc/cc/ccsd_t.h>
#include <mpqc/chemistry/qc/cc/dbccsd.h>
#include <mpqc/chemistry/qc/mbpt/mp2.h>
#include <mpqc/chemistry/qc/mbpt/dbmp2.h>
#include <mpqc/chemistry/qc/f12/f12_utility.h>
#include <mpqc/chemistry/qc/f12/mp2f12.h>
#include <mpqc/chemistry/qc/f12/dbmp2f12.h>
#include <mpqc/chemistry/qc/f12/ccsdf12.h>
#include "../utility/trange1_engine.h"
#include "../ta_routines/array_to_eigen.h"
#include <mpqc/chemistry/qc/scf/soad.h>
#include <mpqc/chemistry/qc/integrals/atomic_integral.h>
#include <mpqc/chemistry/qc/integrals/molecular_integral.h>

using namespace mpqc;
namespace ints = integrals;

TA::TensorD ta_pass_through(TA::TensorD &&ten) { return std::move(ten); }
/**
 *
 *  Example of Main MPQC file
 *
 *  @param Molecule string, path to xyz file, default none
 *  @param GhostMolecule string, path to ghost molecule xyz file, default none
 *  @param NCluster int, number of cluster to use, default 0
 *  @param Charge int, charge of molecule, default 0
 *  @param Basis string, name of basis function, default cc-pvdz
 *  @param DfBasis string, name of density fitting basis function, default none
 *  @param AuxBasis string, name of auxilary basis function, default, none
 *  @param Sparse double,  sparse threashhold, default 1e-13
 *  @param CorrelationFactor double, f12 correlation factor, default by basis name
 *  @param CorrelationFunction int, number of f12 correlation fuction, defualt 6
 *  @param CLSCF object, ClosedShellSCF class
 *  @param AOIntegral object, AtomicIntegral class
 *  @param MOIntegral object, MolecularIntegral class
 *
 */
int try_main(int argc, char *argv[], madness::World &world) {

    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " <input_file.json>" << std::endl;
        throw std::invalid_argument("no input file given");
    }


    char* json;
    utility::parallel_read_file(world, argv[1], json);

    // parse the input
    rapidjson::Document in;
    in.Parse(json);
    delete[] json;

    std::cout.sync_with_stdio(true);
    std::cerr.sync_with_stdio(true);
    std::cout << std::setprecision(15);
    std::wcout.sync_with_stdio(false);
    std::wcout.imbue(std::locale("en_US.UTF-8"));

    /**
     * obtain basis option for program
     */

    if (!in.HasMember("Molecule") || !in.HasMember("NCluster")) {
        if (world.rank() == 0) {
            std::cout << "At a minimum your input file must provide\n";
            std::cout << "\"Molecule\", which is path to an xyz input\n";
        }
    }

    // get Molecule info
    std::string mol_file = in["Molecule"].GetString();
    std::string ghost_atoms = in.HasMember("GhostMolecule") ? in["GhostMolecule"].GetString() : "";
    int charge = in.HasMember("Charge") ? in["Charge"].GetInt() : 0;

    // get cluster info
    int nclusters = in.HasMember("NCluster") ? in["NCluster"].GetInt() : 1;
    std::size_t ao_blocksize = in.HasMember("AOBlockSize") ? in["AOBlockSize"].GetInt() : 0;


    // get basis info
    std::string basis_name = in.HasMember("Basis") ? in["Basis"].GetString() : "cc-pvdz";
    std::string df_basis_name = in.HasMember("DfBasis") ? in["DfBasis"].GetString() : "";
    std::string aux_basis_name = in.HasMember("AuxBasis") ? in["AuxBasis"].GetString() : "";
    std::string vir_basis_name = in.HasMember("VirtualBasis") ? in["VirtualBasis"].GetString() : "";

    // Get thresh info
    auto threshold = in.HasMember("Sparse") ? in["Sparse"].GetDouble() : 1e-15;
    TiledArray::SparseShape<float>::threshold(threshold);
    // print out basis options
    if(world.rank() == 0){
        std::cout << "Molecule: " << mol_file << std::endl;
        utility::print_file(world,mol_file);
        std::cout << "N Cluster: " << nclusters << std::endl;
        std::cout << "Charge: " << charge << std::endl;
        std::cout << "OBS: " << basis_name << std::endl;
        std::cout << "DFBS: " << df_basis_name << std::endl;
        std::cout << "AUXBS: " << aux_basis_name << std::endl;
        std::cout << "AO Block Size: " << ao_blocksize << std::endl;
        std::cout << "Sparse Threshold: " << threshold << std::endl;
    }

    /**
     * Construct Molecule
     */

    char* xyz_file_buffer;
    utility::parallel_read_file(world,mol_file,xyz_file_buffer);
    std::stringstream xyz_file_stream;
    xyz_file_stream << xyz_file_buffer;
    delete[] xyz_file_buffer;

    using molecule::Molecule;
    Molecule mol;
    // construct molecule
    if(nclusters==0){
        mol = Molecule(xyz_file_stream, false);
    }else{
        mol = Molecule(xyz_file_stream, true);
    }
    auto occ = mol.occupation(charge);
    auto repulsion_energy = mol.nuclear_repulsion();
    utility::print_par(world, "Nuclear repulsion_energy = ", repulsion_energy, "\n");


    /**
     * Construct Clustered Molecule, which is used to construct Basis
     */
    molecule::Molecule clustered_mol;
    // if no ghost molecule
    if(ghost_atoms.empty()){
        utility::print_par(world, "Ghost Atom file: None", "\n");
        if(nclusters==0){
            clustered_mol = mol;
        }else{
            clustered_mol = molecule::attach_hydrogens_and_kmeans(mol.clusterables(), nclusters);
        }
    }
    // if has ghost molecule
    else{
        char* ghost_xyz_buffer;
        utility::parallel_read_file(world,ghost_atoms,ghost_xyz_buffer);
        std::stringstream ghost_xyz_stream;
        ghost_xyz_stream << ghost_xyz_buffer;
        delete[] ghost_xyz_buffer;

        utility::print_par(world, "Ghost Atom file: ", ghost_atoms, "\n");
        utility::print_file(world,ghost_atoms);

        auto ghost_molecue = Molecule(ghost_xyz_stream, false);
        auto ghost_elements = ghost_molecue.clusterables();

        // combine both molecule
        auto mol_elements = mol.clusterables();
        mol_elements.insert(mol_elements.end(), ghost_elements.begin(), ghost_elements.end());


        if(nclusters==0){
            clustered_mol = mpqc::molecule::Molecule(mol_elements, false);
        }
        else{
            clustered_mol = mpqc::molecule::attach_hydrogens_and_kmeans(mol_elements, nclusters);
        }

    }


    /**
     * Construct Basis
     *
     */

    auto bs_registry = std::make_shared<OrbitalBasisRegistry>();
    basis::BasisSet bs(basis_name);
    basis::Basis basis = basis::parallel_construct_basis(world,bs,clustered_mol);
//    std::cout << basis << std::endl;
    if(ao_blocksize!=0){
        basis = reblock(basis, cc::reblock_basis, ao_blocksize);
    }
    utility::parallel_print_range_info(world, basis.create_trange1(), "OBS Basis");
    bs_registry->add(OrbitalIndex(L"κ"),basis);

    basis::Basis df_basis;
    if(!df_basis_name.empty()){
        basis::BasisSet dfbs(df_basis_name);
        df_basis = basis::parallel_construct_basis(world,dfbs,clustered_mol);
        if(ao_blocksize!=0){
            df_basis = reblock(df_basis, cc::reblock_basis, ao_blocksize);
        }
        utility::parallel_print_range_info(world, df_basis.create_trange1(), "DF Basis");
        bs_registry->add(OrbitalIndex(L"Κ"),df_basis);
    }

    basis::Basis vir_basis;
    if(!vir_basis_name.empty()){
        basis::BasisSet vbs(vir_basis_name);
        vir_basis = basis::parallel_construct_basis(world,vbs,clustered_mol);
        if(ao_blocksize!=0){
            vir_basis = reblock(vir_basis, cc::reblock_basis, ao_blocksize);
        }
        utility::parallel_print_range_info(world, vir_basis.create_trange1(), "Virtual Basis");
        bs_registry->add(OrbitalIndex(L"Α"),vir_basis);
//        std::cout << vir_basis << std::endl;
    }

    basis::Basis abs_basis;
    basis::Basis ri_basis;
    if(!aux_basis_name.empty()){
        basis::BasisSet abs(aux_basis_name);
        abs_basis = basis::parallel_construct_basis(world,abs,clustered_mol);
        if(ao_blocksize!=0){
            abs_basis = reblock(abs_basis, cc::reblock_basis, ao_blocksize);
        }
        utility::parallel_print_range_info(world, abs_basis.create_trange1(), "AUX Basis");
        bs_registry->add(OrbitalIndex(L"α"), abs_basis);

        // WARNING, RIBS will differ for different approach
        if(vir_basis_name.empty()){
            ri_basis = basis.join(abs_basis);
        }
        else{
            ri_basis = vir_basis.join(abs_basis);
        }

        if(ao_blocksize != 0){
            ri_basis = reblock(ri_basis, cc::reblock_basis, ao_blocksize);
        }
        utility::parallel_print_range_info(world, ri_basis.create_trange1(), "RI Basis");
        bs_registry->add(OrbitalIndex(L"ρ"), ri_basis);
    }

    /**
     * Deal with f12 parameters
     *
     */

    f12::GTGParams gtg_params;
    int n_functions = in.HasMember("CorrelationFunction") ? in["CorrelationFunction"].GetInt() : 6;
    double f12_factor;
    // if user provide factor, use that
    if(in.HasMember("CorrelationFactor")){
        f12_factor = in["CorrelationFactor"].GetDouble();
        gtg_params = f12::GTGParams(f12_factor, n_functions);
    }
    // if not use basis name to get factor
    else{
        gtg_params = f12::GTGParams(basis_name,n_functions);
    }

    std::vector<std::pair<double,double>> param;

    if(!aux_basis_name.empty()){
        param = gtg_params.compute();
        if(world.rank() == 0){
            std::cout << "F12 Correlation Factor: " << gtg_params.exponent << std::endl;
            std::cout << "NFunction: " << gtg_params.n_fit << std::endl;
            std::cout << "F12 Exponent Coefficient" << std::endl;
            for(auto& pair : param){
                std::cout << pair.first << " " << pair.second << std::endl;
            }
            std::cout << std::endl;
        }
    }
    /**
     * Start Caculation Here
     */


    /// initialize AO integral
    libint2::initialize();

    auto ao_in = json::get_nested(in, "AOIntegral");

    integrals::AtomicIntegral<TA::TensorD, TA::SparsePolicy> ao_int
                (world,
                ta_pass_through,
                std::make_shared<molecule::Molecule>(clustered_mol),
                bs_registry,
                param,
                ao_in);


    /**
     * CLSCF
     */
    double scf_energy;
    if(in.HasMember("CLSCF")){
        auto scf_time0 = mpqc_time::fenced_now(world);

        auto scf_in = json::get_nested(in, "CLSCF");
        double scf_converge = scf_in.HasMember("SCFConverge") ? scf_in["SCFConverge"].GetDouble() : 1.0e-7;
        int scf_max_iter = scf_in.HasMember("SCFMaxIter") ? scf_in["SCFMaxIter"].GetInt() : 30;


        // Overlap ints
        auto S = ao_int.compute(L"<κ|λ>");
        // H core int
        auto H = ao_int.compute(L"<κ|H|λ>");

        // emultipole integral
        const auto bs_array = utility::make_array(basis, basis);
        auto multi_pool = ints::make_engine_pool(libint2::Operator::emultipole1, utility::make_array_of_refs(basis));
        auto r_xyz = ints::sparse_xyz_integrals(world, multi_pool, bs_array);

        /// deal with fock builder
        std::string fock_method = scf_in.HasMember("FockBuilder") ? scf_in["FockBuilder"].GetString() : "df";
        std::unique_ptr<scf::FockBuilder> f_builder;
        if(fock_method=="df"){
            auto inv = ao_int.compute(L"( Κ | G| Λ )");
            auto eri3 = ao_int.compute(L"( Κ | G|κ λ)");
            scf::DFFockBuilder<decltype(eri3)> builder(inv, eri3);
            f_builder = make_unique<decltype(builder)>(std::move(builder));
        }
        else if(fock_method=="four center"){

            auto eri4 = ao_int.compute(L"<μ ν| G|κ λ>");
            eri4("mu,nu,kappa, lambda") = eri4("mu,kappa,nu,lambda");
            auto builder = scf::FourCenterBuilder<decltype(eri4)>(std::move(eri4));
            f_builder = make_unique<decltype(builder)>(std::move(builder));
        }


        /// deal with density builder
        std::string density_method = scf_in.HasMember("DensityBuilder") ? scf_in["DensityBuilder"].GetString() : "cholesky";
        std::unique_ptr<scf::DensityBuilder> d_builder;
        if(density_method == "purification"){

            auto db = scf::PurificationDensityBuilder(S, r_xyz, occ/2, std::max(nclusters,1), 0.0, false);
            d_builder = make_unique<scf::PurificationDensityBuilder>(std::move(db));
        }
        else if(density_method == "cholesky"){
            auto db = scf::ESolveDensityBuilder(S, r_xyz, occ/2, std::max(nclusters,1), 0.0, "cholesky inverse", false);
            d_builder = make_unique<scf::ESolveDensityBuilder>(std::move(db));
        }


        auto time0 = mpqc_time::fenced_now(world);
        auto eri_e = ints::make_engine_pool(libint2::Operator::coulomb, utility::make_array_of_refs(df_basis, basis));
        auto F_soad = scf::fock_from_soad(world, clustered_mol, basis, eri_e, H);
        auto time1 = mpqc_time::fenced_now(world);
        auto time = mpqc_time::duration_in_s(time0, time1);
        mpqc::utility::print_par(world, "Soad Time:  ", time, "\n");

        /// CL scf class
        scf::ClosedShellSCF scf(H, S, repulsion_energy, std::move(f_builder), std::move(d_builder), F_soad);
        scf.solve(scf_max_iter, scf_converge);

        scf_energy = scf.scf_energy();
        auto scf_time1 = mpqc_time::fenced_now(world);
        auto scf_time = mpqc_time::duration_in_s(scf_time0, scf_time1);
        mpqc::utility::print_par(world, "SCF Energy:  ", scf_energy, "\n");
        mpqc::utility::print_par(world, "Total SCF Time:  ", scf_time, "\n");

        auto F = scf.fock();
        if(fock_method == "df"){
            ao_int.registry().insert(Formula(L"<μ|F|ν>[df]"),F);
        }
        else if(fock_method == "four center"){
            ao_int.registry().insert(Formula(L"<μ|F|ν>"),F);
        }
    }
    else {
      throw std::runtime_error("CLSCF object not found in the input file");
    }


    // Correlation Methods

    auto mo_in = json::get_nested(in, "MOIntegral");
    double corr_e = 0.0;
    auto orbital_registry = std::make_shared<OrbitalSpaceRegistry<TA::DistArray<TA::TensorD, TA::SparsePolicy>>>();
    auto mo_integral = integrals::MolecularIntegral<TA::TensorD,TA::SparsePolicy>(ao_int,orbital_registry,mo_in);
    Eigen::VectorXd ens;
    std::shared_ptr<TRange1Engine> tre;
    rapidjson::Document corr_in;

    if(in.HasMember("MP2")){

        auto mp2_time0 = mpqc_time::fenced_now(world);

        corr_in = json::get_nested(in, "MP2");
        auto mp2 = mbpt::MP2<TA::TensorD, TA::SparsePolicy>(mo_integral);
        corr_e += mp2.compute(corr_in);

        auto mp2_time1 = mpqc_time::fenced_now(world);
        auto mp2_time = mpqc_time::duration_in_s(mp2_time0, mp2_time1);
        mpqc::utility::print_par(world, "Total MP2 Time:  ", mp2_time, "\n");
    }
    else if(in.HasMember("DBMP2")){
        auto dbmp2_time0 = mpqc_time::fenced_now(world);

        corr_in = json::get_nested(in, "DBMP2");
        std::shared_ptr<mbpt::MP2<TA::TensorD, TA::SparsePolicy>> mp2 = std::make_shared<mbpt::DBMP2<TA::TensorD, TA::SparsePolicy>>(mbpt::DBMP2<TA::TensorD, TA::SparsePolicy>(mo_integral));
        corr_e += mp2->compute(corr_in);

        auto dbmp2_time1 = mpqc_time::fenced_now(world);
        auto dbmp2_time = mpqc_time::duration_in_s(dbmp2_time0, dbmp2_time1);
        mpqc::utility::print_par(world, "Total DBMP2 Time:  ", dbmp2_time, "\n");

    }
    else if(in.HasMember("MP2F12")){
        corr_in = json::get_nested(in, "MP2F12");

        // start mp2f12
        auto mp2f12_time0 = mpqc_time::fenced_now(world);
        f12::MP2F12<TA::TensorD> mp2f12(mo_integral);
        corr_e += mp2f12.compute(corr_in);

        auto mp2f12_time1 = mpqc_time::fenced_now(world);
        auto mp2f12_time = mpqc_time::duration_in_s(mp2f12_time0, mp2f12_time1);
        mpqc::utility::print_par(world, "Total MP2 F12 Time:  ", mp2f12_time, "\n");
    }
    else if(in.HasMember("DBMP2F12")){
        corr_in = json::get_nested(in, "DBMP2F12");

        // start mp2f12
        auto mp2f12_time0 = mpqc_time::fenced_now(world);
        f12::DBMP2F12<TA::TensorD> dbmp2f12(mo_integral);
        corr_e += dbmp2f12.compute(corr_in);

        auto mp2f12_time1 = mpqc_time::fenced_now(world);
        auto mp2f12_time = mpqc_time::duration_in_s(mp2f12_time0, mp2f12_time1);
        mpqc::utility::print_par(world, "Total MP2 F12 Time:  ", mp2f12_time, "\n");

    }
    // all of these require CCSD
    else if(in.HasMember("CCSD") || in.HasMember("CCSD(T)") || in.HasMember("CCSD(F12)") || in.HasMember("DBCCSD")) {

        auto time0 = mpqc_time::fenced_now(world);

        if(in.HasMember("CCSD")){
            utility::print_par(world, "\nBegining CCSD Calculation\n");
            corr_in = json::get_nested(in, "CCSD");
            mpqc::cc::CCSD<TA::TensorD, TA::SparsePolicy> ccsd(mo_integral, corr_in);
            corr_e += ccsd.compute();
            auto time1 = mpqc_time::fenced_now(world);
            auto time = mpqc_time::duration_in_s(time0, time1);
            mpqc::utility::print_par(world, "Total CCSD Time:  ", time, "\n");
        }
        else if(in.HasMember("DBCCSD")){
            utility::print_par(world, "\nBegining Dual Basis CCSD Calculation\n");
            corr_in = json::get_nested(in, "DBCCSD");
            mpqc::cc::DBCCSD<TA::TensorD, TA::SparsePolicy> dbccsd(mo_integral, corr_in);
            corr_e += dbccsd.compute();
            auto time1 = mpqc_time::fenced_now(world);
            auto time = mpqc_time::duration_in_s(time0, time1);
            mpqc::utility::print_par(world, "Total Dual Basis CCSD Time:  ", time, "\n");
        }
        else if(in.HasMember("CCSD(T)")){
            utility::print_par(world, "\nBegining CCSD(T) Calculation\n");
            corr_in = json::get_nested(in, "CCSD(T)");
            mpqc::cc::CCSD_T<TA::TensorD, TA::SparsePolicy> ccsd_t(mo_integral, corr_in);
            corr_e += ccsd_t.compute();
            auto time1 = mpqc_time::fenced_now(world);
            auto time = mpqc_time::duration_in_s(time0, time1);
            mpqc::utility::print_par(world, "Total CCSD(T) Time:  ", time, "\n");
        }

        if(in.HasMember("CCSD(F12)")){

            time0 = mpqc_time::fenced_now(world);

            utility::print_par(world, "\nBegining CCSD(F12) Calculation\n");
            corr_in = json::get_nested(in, "CCSD(F12)");

            corr_in = json::get_nested(in, "CCSD(F12)");

            f12::CCSDF12<TA::TensorD> ccsd_f12(mo_integral, corr_in);
            corr_e += ccsd_f12.compute();

            auto time1 = mpqc_time::fenced_now(world);
            auto time = mpqc_time::duration_in_s(time0, time1);
            mpqc::utility::print_par(world, "Total F12 Time:  ", time, "\n");

        }
    }

    utility::print_par(world, "Total Correlation Energy: ", corr_e, "\n");
    utility::print_par(world, "Total Energy: ", corr_e + scf_energy, "\n");

    world.gop.fence();
    libint2::finalize();
    return 0;
}


int main(int argc, char *argv[]) {

    int rc = 0;

    auto &world = madness::initialize(argc, argv);
    mpqc::utility::print_par(world, "MADNESS process total size: ", world.size(), "\n");

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
