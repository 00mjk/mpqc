#pragma once

#include "tensor_fwd.h"
#include "decomp_helper.h"
#include <vector>
#include <algorithm>
#include <numeric>
#include <tuple>


namespace tcc {
namespace tensor {

/**
 * Class which specializes in holding an Exchange Tensor.
 **/
class ExchangeTensor {
  private:
    TARange range_;
    double cut_;
    std::vector<TATensor> tensors_;

  public:
    using value_type = double;
    using numeric_type = double;

  public:
    // Compiler provided constructors
    ExchangeTensor() = default;
    ExchangeTensor(ExchangeTensor const &) = default;
    ExchangeTensor(ExchangeTensor &&) = default;
    ExchangeTensor &operator=(ExchangeTensor const &) = default;
    ExchangeTensor &operator=(ExchangeTensor &&) = default;

    ExchangeTensor(TARange const &r, double c) : range_(r), cut_(c) {}
    ExchangeTensor(TARange const &r, TATensor t, double c)
        : range_(r), cut_(c), tensors_{std::move(t)} {}

    ExchangeTensor(TARange const &r, TATensor t1, TATensor t2, TATensor t3,
                   double c)
        : range_(r),
          cut_(c),
          tensors_{std::move(t1), std::move(t2), std::move(t3)} {}

    ExchangeTensor(TARange const &r, std::vector<TATensor> tvec, double c)
        : range_(r), cut_(c), tensors_{std::move(tvec)} {}

    ExchangeTensor(std::tuple<TARange, std::vector<TATensor>> const &tup,
                   double c)
        : range_(std::get<0>(tup)), cut_(c), tensors_(std::get<1>(tup)) {}

    ExchangeTensor(ExchangeTensor const &other, Permutation const &perm) {}

    // Normal Tiledarary tensor functions
    const TARange &range() const { return range_; }
    unsigned long size() const { return range_.volume(); }
    bool empty() const { return tensors_.empty(); }

    // Functions for the low rank tensor.
    bool is_decomposed() const { return tensors_.size() >= 2; }

    std::vector<unsigned int> ranks() const {
        assert(!tensors_.empty());
        std::vector<unsigned int> ranks;

        if (tensors_.size() == 1) {
            ranks.push_back(range_.size()[0] * range_.size()[1]);
        } else {
            assert(tensors_.size() == 3);
            ranks.push_back(tensors_[0].range().size()[1]);
            ranks.push_back(tensors_[2].range().size()[0]);
        }

        return ranks;
    }


    unsigned long compressed_size() const {
        return std::accumulate(std::begin(tensors_), std::end(tensors_), 0,
                               [](unsigned long int size, TATensor const &t) {
            return size + t.range().volume();
        });
    }

    double cut() const { return cut_; }

    template <typename Archive>
    typename mad::enable_if<mad::archive::is_output_archive<Archive>>::type
    serialize(Archive &ar) {}

    /// Serialize tensor data
    template <typename Archive>
    typename mad::enable_if<mad::archive::is_input_archive<Archive>>::type
    serialize(Archive &ar) {}

    void swap(ExchangeTensor &other) {
        using std::swap;
        swap(tensors_, other.tensors_);
        swap(range_, other.range_);
    }

    ExchangeTensor permute(Permutation const &perm) const {
        return ExchangeTensor(*this, perm);
    }

    template <typename Op>
    ExchangeTensor binary(ExchangeTensor const &other, Op const &op) const {
        return ExchangeTensor(*this, other, op);
    }

    template <typename Op>
    ExchangeTensor
    binary(ExchangeTensor &other, const Op &op, const Permutation &perm) const {
        return ExchangeTensor(*this, other, op, perm);
    }

    template <typename Op>
    ExchangeTensor unary(const Op &op) const {
        return ExchangeTensor(*this, op);
    }

    template <typename Op>
    ExchangeTensor unary(const Op &op, const Permutation &perm) const {
        return ExchangeTensor(*this, op, perm);
    }

    template <typename Op>
    ExchangeTensor &inplace_unary(const Op &op) {
        assert(false);
    }

    ExchangeTensor scale(double factor) const { assert(false); }

    ExchangeTensor scale(double factor, const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor &scale_to(double factor) { assert(false); }

    ExchangeTensor add(const ExchangeTensor &other) const {
        return this->add(other, 1.0);
    }

    ExchangeTensor
    add(const ExchangeTensor &other, const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor add(const ExchangeTensor &other, double factor) const {
        assert(false);
    }

    ExchangeTensor add(const ExchangeTensor &other, double factor,
                       const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor add(double value) const { assert(false); }

    ExchangeTensor add(double value, const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor &add_to(const ExchangeTensor &other) { assert(false); }

    template <typename U, typename AU>
    ExchangeTensor &add_to(const ExchangeTensor &other, double factor) {
        assert(false);
    }

    ExchangeTensor &add_to(double value) { assert(false); }

    template <typename U, typename AU>
    ExchangeTensor subt(const ExchangeTensor &other) const {
        assert(false);
    }

    template <typename U, typename AU>
    ExchangeTensor
    subt(const ExchangeTensor &other, const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor subt(const ExchangeTensor &other, double factor) const {
        assert(false);
    }

    ExchangeTensor subt(const ExchangeTensor &other, double factor,
                        const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor subt(double value) const { assert(false); }

    ExchangeTensor subt(double value, const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor &subt_to(const ExchangeTensor &other) { assert(false); }

    template <typename U, typename AU>
    ExchangeTensor &subt_to(const ExchangeTensor &other, double factor) {
        assert(false);
    }

    ExchangeTensor &subt_to(double value) { assert(false); }

    ExchangeTensor mult(const ExchangeTensor &other) const { assert(false); }

    ExchangeTensor
    mult(const ExchangeTensor &other, const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor mult(const ExchangeTensor &other, double factor) const {
        assert(false);
    }

    template <typename U, typename AU>
    ExchangeTensor mult(const ExchangeTensor &other, double factor,
                        const Permutation &perm) const {
        assert(false);
    }

    ExchangeTensor &mult_to(const ExchangeTensor &other) { assert(false); }

    ExchangeTensor &mult_to(const ExchangeTensor &other, double factor) {
        assert(false);
    }

    ExchangeTensor neg() const { assert(false); }

    ExchangeTensor neg(const Permutation &perm) const { assert(false); }

    ExchangeTensor &neg_to() { assert(false); }

    ExchangeTensor gemm(const ExchangeTensor &other, double factor,
                        const math::GemmHelper &gemm_helper) const {
        assert(false);
    }

    ExchangeTensor &gemm(const ExchangeTensor &left,
                         const ExchangeTensor &right, double factor,
                         const math::GemmHelper &gemm_helper) {
        assert(false);
    }

    double trace() const { assert(false); }

  public:
    template <typename Op>
    double reduce(double init_value, const Op &op) const {
        assert(false);
    }

    template <typename Op>
    double
    reduce(const ExchangeTensor &other, double init_value, const Op &op) const {
        assert(false);
    }

    double sum() const { assert(false); }

    double product() const { assert(false); }

    double squared_norm() const { assert(false); }

    double norm() const {
        if (tensors_.empty()) {
            return 0.0;
        } else {
            auto sum = 0.0;
            for (auto const &t : tensors_) {
                if(!t.empty()){
                    sum += t.norm();
                }
            }
            return sum;
        }
    }

    double min() const { assert(false); }

    double max() const { assert(false); }

    double abs_min() const { assert(false); }

    double abs_max() const { assert(false); }

    double dot(const ExchangeTensor &other) const { assert(false); }
};

inline ExchangeTensor
operator+(const ExchangeTensor &left, const ExchangeTensor &right) {
    assert(false);
}

inline ExchangeTensor
operator-(const ExchangeTensor &left, const ExchangeTensor &right) {
    assert(false);
}

inline ExchangeTensor
operator*(const ExchangeTensor &left, const ExchangeTensor &right) {
    assert(false);
}

inline ExchangeTensor operator^(const Permutation &perm,
                                const ExchangeTensor &tensor) { assert(false); }

inline std::ostream &
operator<<(std::ostream &os, const ExchangeTensor &t) {
    assert(false);
}

} // namespace tensor
} // namespace tcc
