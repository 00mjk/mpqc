
#include "molecule.h"
#include "cluster_concept.h"
#include "cluster.h"
#include "../include/tbb.h"
#include "common.h"
#include <functional>

namespace molecule_detail {

inline double calculate_mass(const std::vector<Clusterable> &cs) {
    using iter_t = decltype(cs.begin());
    return tbb::parallel_reduce(
        tbb::blocked_range<iter_t>(cs.begin(), cs.end()), 0.0,
        [](const tbb::blocked_range<iter_t> & r, double d)->double {
            return std::accumulate(
                r.begin(), r.end(), d,
                [](double x, const Clusterable &c) { return x + c.mass(); });
        },
        std::plus<double>());
}

inline double calculate_charge(const std::vector<Clusterable> &cs) {

    using iter_t = decltype(cs.begin());
    return tbb::parallel_reduce(
        tbb::blocked_range<iter_t>(cs.begin(), cs.end()), 0.0,
        [](const tbb::blocked_range<iter_t> & r, double d)->double {
            return std::accumulate(
                r.begin(), r.end(), d,
                [](double x, const Clusterable &c) { return x + c.charge(); });
        },
        std::plus<double>());

}

// Functor for sorting centers based on the distance from a point.
class sort_by_distance_from_point {
  public:
    sort_by_distance_from_point(const Cluster::position_t point)
        : point_(point) {}

    bool operator()(const Clusterable &a, const Clusterable &b) const {
        Atom::position_t a_dist = a.center() - point_;
        Atom::position_t b_dist = b.center() - point_;
        if (!(a_dist.squaredNorm() == b_dist.squaredNorm())) {
            return a_dist.squaredNorm() < b_dist.squaredNorm();
        } else if (a_dist[0] == b_dist[0]) {
            return a_dist[0] < b_dist[0];
        } else if (a_dist[1] == b_dist[1]) {
            return a_dist[1] < b_dist[1];
        } else
            return a_dist[2] < b_dist[2];
    }

  private:
    Clusterable::position_t point_;
};

void sort_elements(std::vector<Clusterable> &elems, const position_t &point) {
    tbb::parallel_sort(elems.begin(), elems.end(),
                       sort_by_distance_from_point(point));
}
} // namespace moleucle detail

Molecule::Molecule(std::vector<Clusterable> c) : elements_(std::move(c)) {
    mass_ = molecule_detail::calculate_mass(elements_);
    charge_ = molecule_detail::calculate_charge(elements_);
    center_ = center_of_mass(elements_, mass_);
    molecule_detail::sort_elements(elements_, center_);
}