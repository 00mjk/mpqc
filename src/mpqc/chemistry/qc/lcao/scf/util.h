
#ifndef MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_UTIL_H_
#define MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_UTIL_H_

#include <array>
#include <vector>

namespace mpqc {
namespace utility {

template <typename T>
T vec_avg(std::vector<T> const &v) {
  T sum = 0;
  for (auto const &c : v) {
    sum += c;
  }

  return sum / T(v.size());
}

inline std::array<double, 3> vec_avg(
    std::vector<std::array<double, 3>> const &v) {
  std::array<double, 3> sum = {{0, 0, 0}};
  for (auto const &c : v) {
    sum[0] += c[0];
    sum[1] += c[1];
    sum[2] += c[2];
  }

  sum[0] /= double(v.size());
  sum[1] /= double(v.size());
  sum[2] /= double(v.size());

  return sum;
}
}
}

#endif  // MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_UTIL_H_
