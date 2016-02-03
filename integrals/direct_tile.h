#pragma once
#ifndef MPQC_INTEGRALS_DIRECTTILE_H
#define MPQC_INTEGRALS_DIRECTTILE_H

#include "task_integrals_common.h"

// #include "../tensor/tcc_tile.h"

#include "../include/tiledarray.h"

#include <madness/world/worldptr.h>

#include <memory>
#include <vector>
#include <functional>

namespace mpqc {
namespace integrals {

/*! \brief A direct tile for integral construction
 *
 */
template <typename Builder>
class DirectTile {
  private:
    std::vector<std::size_t> idx_;
    TA::Range range_;
    std::shared_ptr<Builder> builder_;

    using TileType = decltype(std::declval<Builder>()(
          std::declval<std::vector<std::size_t>>(), std::declval<TA::Range>()));

  public:
    using value_type = double;
    using eval_type = TileType;
    using range_type = TA::Range;

    DirectTile() = default;
    DirectTile(DirectTile const &) = default;
    DirectTile(DirectTile &&) = default;
    DirectTile &operator=(DirectTile const &) = default;
    DirectTile &operator=(DirectTile &&) = default;

    DirectTile(std::vector<std::size_t> index, TA::Range range,
               std::shared_ptr<Builder> builder)
            : idx_(std::move(index)),
              range_(std::move(range)),
              builder_(std::move(builder))
    {}

    operator eval_type() const { return builder_->operator()(idx_, range_); }

#if 0
    template <typename Archive>
    void serialize(Archive &) {
        assert(false);
    }

#else

    template <typename Archive>
    enable_if_t<madness::archive::is_output_archive<Archive>::value, void>
    serialize(Archive &ar) {
        ar &idx_;
        ar &range_;
        assert(builder_ != nullptr);
        ar & builder_->id();
    }

    template <typename Archive>
    enable_if_t<madness::archive::is_input_archive<Archive>::value, void>
    serialize(Archive &ar) {
        ar &idx_;
        ar &range_;
        madness::uniqueidT id;
        ar &id;

        assert(builder_ == nullptr);
        madness::World *world
              = madness::World::world_from_id(id.get_world_id());
        builder_ = world->template shared_ptr_from_id<Builder>(id);
        assert(builder_ != nullptr);
    }
#endif
};


/*! Class to hold a direct tile builder with its array. */
template <typename Builder, typename Array>
class DirectArray {
  private:
    std::shared_ptr<Builder> builder_;
    Array array_;

  public:
    DirectArray() = default;
    DirectArray(std::shared_ptr<Builder> b, Array a)
            : builder_(std::move(b)), array_(std::move(a)) {}

    DirectArray(std::shared_ptr<Builder> b) : builder_(std::move(b)), array_() {}

    template <typename... Args>
    auto operator()(Args &&... args)
          -> decltype(array_(std::forward<Args>(args)...)) {
        return array_(std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto operator()(Args &&... args) const
          -> decltype(array_(std::forward<Args>(args)...)) {
        return array_(std::forward<Args>(args)...);
    }

    void set_array(Array a) { array_ = std::move(a); }

    Array &array() { return array_; }

    Array const &array() const { return array_; }

    std::shared_ptr<Builder> builder() { return builder_; }

    const std::shared_ptr<Builder> builder() const { return builder_; }
};

} // namespace integrals
} // namespace mpqc

#endif // MPQC_INTEGRALS_DIRECTTILE_H
