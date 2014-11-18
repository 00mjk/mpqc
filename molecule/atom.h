#pragma once
#ifndef TCC_MOLECULE_ATOM_H
#define TCC_MOLECULE_ATOM_H

#include "../include/eigen.h"

namespace tcc {
namespace molecule {

class Atom {
  public:
    using position_t = Eigen::Vector3d;

    Atom() = default;
    Atom(const Atom &atom) = default;
    Atom &operator=(const Atom &atom) = default;

    // Can't move eigen types yet so use eigen's swap.
    Atom(Atom &&atom) noexcept : charge_(std::move(atom.charge_)),
                                 mass_(std::move(atom.mass_)) {
        atom.center_.swap(center_);
    }

    Atom &operator=(Atom &&atom) noexcept {
        charge_ = std::move(atom.charge_);
        mass_ = std::move(atom.mass_);
        atom.center_.swap(center_);
        return *this;
    }

    Atom(position_t center, double mass, double charge)
        : center_(std::move(center)),
          charge_(charge),
          mass_(mass) {}

    position_t center() const { return center_; }
    double charge() const { return charge_; }
    double mass() const { return mass_; }

  private:
    position_t center_ = {0, 0, 0};
    double charge_ = 0;
    double mass_ = 0;
};

} // namespace molecule
} // namespace tcc

#endif // TCC_MOLECULE_ATOM_H