
#ifndef MPQC4_SRC_MPQC_CHEMISTRY_QC_INTEGRALS_SCREENING_QVL_SHELL_INFO_H_
#define MPQC4_SRC_MPQC_CHEMISTRY_QC_INTEGRALS_SCREENING_QVL_SHELL_INFO_H_


#include "mpqc/chemistry/qc/lcao/integrals/task_integrals_common.h"

namespace mpqc {
namespace lcao {
namespace gaussian {
namespace detail {

/*! \brief Class to hold information for QVl aux basis type screening.
 */
class QVlShellInfo {
  private:
    std::vector<double> a_extents_;
    RowMatrixXd cd_extents_;

    std::vector<Vector3d> a_centers_;
    std::vector<std::vector<Vector3d>> cd_centers_;

    std::vector<double> a_factor_;
    std::vector<double> a_am_;
    RowMatrixXd one_over_gamma_ab_pow_;

    double erfinv_thr_;

  public:
    QVlShellInfo() = default;
    QVlShellInfo(ShellVec const &a_shells, ShellVec const &cd_shells,
                 double thresh);

    double a_extent(int64_t i) const { return a_extents_[i]; }
    Vector3d const &a_center(int64_t i) const { return a_centers_[i]; }

    double cd_extent(int64_t i, int64_t j) const { return cd_extents_(i, j); }
    Vector3d const &cd_center(int64_t i, int64_t j) const {
        return cd_centers_[i][j];
    }

    // Returns the factor that scales Qab in David's screening paper.
    double
    scaling_factor(int64_t a, int64_t c, int64_t d, double Rnorm) const {
        const auto rscale = 1 / (std::pow(Rnorm, a_am_[a] + 1));
        return rscale * a_factor_[a] * one_over_gamma_ab_pow_(c,d);
    }

  private:
    double pair_extent(Shell const &sh0, Shell const &sh1, Vector3d const &r_01);
    double a_shell_min_exp(Shell const &sh);
    Vector3d shell_weighted_center(Shell const &sh0, Shell const &sh1) const;
};

}  // namespace  detail
}  // namespace  gaussian
}  // namespace  lcao
}  // namespace  mpqc

#endif  // MPQC4_SRC_MPQC_CHEMISTRY_QC_INTEGRALS_SCREENING_QVL_SHELL_INFO_H_
