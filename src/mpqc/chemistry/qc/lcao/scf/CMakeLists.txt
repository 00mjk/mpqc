set(sources
builder.h
cadf_builder.h
cadf_builder_forced_shape.h
cadf_fitting_coeffs.h
cadf_helper_functions.h
clr_cadf_builder.h
clusterd_coeffs.h
density_builder.h
eigen_solve_density_builder.h
eigen_solve_density_builder_impl.h
four_center_exact_k_diagonal_builder.h
linkage.h
mo_build.h
orbital_localization.cpp
orbital_localization.h
purification_density_build.h
purification_density_build_impl.h
purification/purification_devel.h
rhf.cpp
rhf.h
rhf_impl.h
rij_exact_k_fock_builder.h
soad.h
ta_shape_tracker.h
traditional_df_fock_builder.h
traditional_four_center_fock_builder.h
util.h
)

if(TA_DEFAULT_POLICY EQUAL 1)
list(APPEND sources
        zrhf.h
        zrhf_impl.h
        zrhf.cpp
        decomposed_rij.h
        decomposed_rij.cpp
        pbc/periodic_cond_ortho.h
        pbc/periodic_soad.h
        pbc/periodic_two_center_builder.h
        pbc/periodic_three_center_contraction_builder.h
        pbc/periodic_four_center_fock_builder.h
        pbc/periodic_ri_j_builder.h
        pbc/periodic_df_fock_builder.h
        pbc/periodic_cadf_k_builder.h
        pbc/periodic_ri_j_cadf_k_fock_builder.h
        pbc/periodic_ma.h
        pbc/periodic_ma.cpp
        pbc/periodic_ma_ri_j_builder.h
        pbc/periodic_ma_ri_j_cadf_k_fock_builder.h
        pbc/periodic_ma_ri_j_four_center_k_fock_builder.h
        pbc/periodic_ma_four_center_fock_builder.h
        pbc/periodic_ma_four_center_j_cadf_k_fock_builder.h
        pbc/periodic_four_center_j_cadf_k_fock_builder.h
        pbc/util.h
        )
endif()

add_mpqc_library(lcao_scf sources sources "MPQCproperties;MPQClcao_expression;MPQClcao_wfn;MPQCmath_ta;MPQCmath_linalg;MPQCmath_clr" "mpqc/chemistry/qc/lcao/scf")
