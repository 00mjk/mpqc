#ifndef SRC_MPQC_CHEMISTRY_QC_CC_SOLVERS_H_
#define SRC_MPQC_CHEMISTRY_QC_CC_SOLVERS_H_

#include "diis.h"
#include "mpqc/util/keyval/keyval.h"
#include "mpqc/util/misc/exception.h"

namespace mpqc {
namespace cc {

/// Solver updates the CC T amplitudes given the current values
/// and the amplitude equation residuals
/// @tparam T the type representing the n-body amplitude set
template <typename T>
class Solver {
 public:
  virtual ~Solver() = default;

  /// Updates amplitudes \c t1 and \c t2 using the residuals \c r1 and \c r2 .
  /// @warning This function assumes that {\c t1 , \c t2 } and {\c r1 , \c r2 }
  ///          are congruent, but does not assume any particular structure.
  ///          Derived classes \em may impose additional assumptions on the
  ///          structure of the arguments.
  /// @param t1 the 1-body amplitude set (current values on input, updated
  /// values on output)
  /// @param t2 the 2-body amplitude set (current values on input, updated
  /// values on output)
  /// @param r1 the 1-body amplitude equation residual set (contents may be
  /// modified)
  /// @param r2 the 2-body amplitude equation residual set (contents may be
  /// modified)
  virtual void update(T& t1, T& t2, const T& r1, const T& r2) = 0;
  virtual void update(T& t1, T& t2, T& t3, const T& r1, const T& r2, const T& r3) = 0;
};

/// DIISSolver updates the CC T amplitudes using DIIS
/// @tparam T the type representing the n-body amplitude set
template <typename T>
class DIISSolver : public Solver<T> {
 public:
  // clang-format off
  /**
   * @brief The KeyVal constructor.
   *
   * @param kv the KeyVal object; it will be queried for the following keywords
   *
   * | Keyword | Type | Default| Description |
   * |---------|------|--------|-------------|
   * | diis_start | int | 1 | The DIIS extrapolation will begin on the iteration given by this integer. |
   * | n_diis | int | 8 | This specifies maximum number of data sets to retain. |
   * | diis_damp | double | 0.0 | This nonnegative floating point number is used to dampen the DIIS extrapolation. |
   * | diis_ngroup | int | 1 | The number of iterations in a DIIS group. DIIS extrapolation is only used for the first \c diis_group_nstart of these iterations. If \c diis_ngroup is 1 and \c diis_group_nstart is greater than 0, then DIIS will be used on all iterations after and including the start iteration. |
   * | diis_group_nstart | int | 1 | The number of DIIS extrapolations to do at the beginning of an iteration group.  See the documentation for \c diis_ngroup . |
   */
  // clang-format on
  DIISSolver(const KeyVal& kv)
      : diis_(kv.value<int>("diis_start", 1), kv.value<int>("n_diis", 8), kv.value<double>("diis_damp", 0.0),
              kv.value<int>("diis_ngroup", 1), kv.value<int>("diis_group_nstart", 1)) {}
  virtual ~DIISSolver() = default;

  /// Update the amplitudes using update_only() and extrapolate using DIIS.
  /// @warning This function assumes that {\c t1 , \c t2 } and {\c r1 , \c r2 }
  ///          are congruent, but does not assume any particular structure.
  ///          Derived classes \em may impose additional assumptions on the
  ///          structure of the arguments.
  /// @param t1 the 1-body amplitude set (current values on input, updated
  /// values on output)
  /// @param t2 the 2-body amplitude set (current values on input, updated
  /// values on output)
  /// @param r1 the 1-body amplitude equation residual set (contents may be
  /// modified)
  /// @param r2 the 2-body amplitude equation residual set (contents may be
  /// modified)
  void update(T& t1, T& t2, const T& r1, const T& r2) override {
    update_only(t1, t2, r1, r2);
    T r1_copy = r1;
    T r2_copy = r2;
    TPack<T> r(r1_copy, r2_copy);
    TPack<T> t(t1, t2);
    diis_.extrapolate(t, r);
    t1 = t[0];
    t2 = t[1];
  }

  void update(T& t1, T& t2,  T& t3, const T& r1, const T& r2, const T& r3) override{
    update_only(t1, t2, t3, r1, r2, r3);
    T r1_copy = r1;
    T r2_copy = r2;
    T r3_copy = r3;
    TPack<T> r(r1_copy, r2_copy, r3_copy);
    TPack<T> t(t1, t2, t3);
    diis_.extrapolate(t, r);
    t1 = t[0];
    t2 = t[1];
    t3 = t[2];
  }

 protected:
  /// this performs the amplitude update only, to be followed up with DIIS
  /// @warning This function assumes that {\c t1 , \c t2 } and {\c r1 , \c r2 }
  ///          are congruent, but does not assume any particular structure.
  ///          Derived classes \em may impose additional assumptions on the
  ///          structure of the arguments.
  virtual void update_only(T& t1, T& t2, const T& r1, const T& r2) {
    throw ProgrammingError("DIISSolver::update_only must be implemented in the derived class",
                           __FILE__, __LINE__);
  }
  virtual void update_only(T& t1, T& t2, T& t3, const T& r1, const T& r2, const T& r3) {
        throw ProgrammingError("DIISSolver::update_only must be implemented in the derived class",
                               __FILE__, __LINE__);
  }


  TA::DIIS<TPack<T>>& diis() { return diis_; }

  private:
  TA::DIIS<TPack<T>> diis_;



 };

}  // namespace cc
}  // namespace mpqc

#endif /* SRC_MPQC_CHEMISTRY_QC_CC_SOLVERS_H_ */
