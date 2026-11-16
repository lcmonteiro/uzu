/// ===============================================================================================
/// @file
///
/// @brief The node base: an estimate the graph is free to move.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_NODE_HPP
#define UZU_OPTIMIZATION_GRAPH_NODE_HPP

#include <array>
#include <cstddef>

#include "uzu/foundation/dual/array.hpp"
#include "uzu/helpers/keywords.hpp"

namespace uzu {

/// @brief Base for a node holding an estimate the fit may move.
///
/// The derived class supplies `plus`, which says how a delta is applied to the estimate. That is
/// the whole of what a node has to know: for a plain vector it is addition, and for anything on a
/// manifold - an angle, a rotation - it is where the wrap or the retraction goes.
///
/// The estimate and the delta are named apart because they are different things - what is held
/// and what moves it - but for now they must be the same width, and the assertion below says so.
/// Seeding goes through the estimate, one dual index per coordinate, while the layout hands out
/// one index per `Dimension`; if those disagreed the extra indices would run past this node's
/// block and alias the next node's. Supporting an estimate wider than its delta - a unit
/// quaternion, four numbers with three directions - means seeding a zero delta instead and
/// pushing it through `plus`, which would make `plus` generic over duals rather than a function
/// returning a concrete estimate. That is a change to what every node author writes, so it is not
/// smuggled in here.
///
/// @tparam Derived The derived node type.
/// @tparam Estimation What the estimate is. Named `estimation_type` inside, and supplies the
/// scalar as its own `value_type` - which is what this class then calls `value_type` too.
/// @tparam Dimension How many numbers `plus` receives - the dimension the graph moves this node
/// in, and the number of dual indices it takes up. Must match the estimate's width; see above.
template <class Derived, class Estimation, auto Dimension>
class node {
 public:
  /// @brief The estimate: what `estimation()` hands back and `plus` returns.
  using estimation_type = Estimation;

  /// @brief The scalar the estimate is made of, taken from the estimate itself.
  using value_type = typename Estimation::value_type;

  /// @brief The delta `plus` receives, and the sigma that scales it.
  using point = std::array<value_type, Dimension>;

  /// @brief How many numbers the graph moves this node in.
  static constexpr std::size_t dimension = Dimension;

  static_assert(
      std::tuple_size_v<Estimation> == dimension,
      "an estimate and the delta that moves it must be the same width for now: the "
      "seeding takes one dual index per estimate coordinate, so a wider estimate would "
      "run past this node's block in the layout");

  /// @brief Builds a node from `init` and `sigma`, in either order.
  template <impl::keyword_argument... Args>
  explicit constexpr node(const Args &...args)
      : estimation_{impl::find<impl::init_tag>(estimation_type{}, args...)},
        sigma_{impl::find_all<impl::sigma_tag>(filled(value_type{1}), args...)} {
    impl::keyword_set<impl::init_tag, impl::sigma_tag>::check<Args...>();
  }

  /// @brief The current estimate.
  constexpr auto estimation() const -> const estimation_type & {
    return estimation_;
  }

  /// @brief Overwrites the current estimate.
  constexpr auto estimation(const estimation_type &value) -> void {
    estimation_ = value;
  }

  /// @brief The scale the graph's kernel normalises this node's gradient against, one per
  /// dimension.
  constexpr auto sigma() const -> const point & {
    return sigma_;
  }

  /// @brief Seeds this node's estimate as duals, one index per dimension.
  ///
  /// Offset is where this node's block starts in the graph's index space, so every node's
  /// derivatives stay distinguishable in the accumulated error.
  template <std::size_t Offset>
  constexpr auto seed() const {
    return dual::make_array<Offset>(estimation_);
  }

  /// @brief Applies a delta through the derived class's `plus`.
  constexpr auto update(const point &delta) -> void {
    estimation_ = static_cast<Derived *>(this)->plus(delta);
  }

  /// @brief Whether the fit may move this node. A disabled node is held where it is: edges still
  /// read its estimate, as a constant, but `fit` neither steps it nor calls the algorithm for its
  /// indices. Nodes start enabled.
  constexpr auto enabled() const -> bool {
    return enabled_;
  }

  /// @brief Lets the fit move this node again.
  constexpr auto enable() -> void {
    enabled_ = true;
  }

  /// @brief Holds this node where it is until it is enabled again.
  constexpr auto disable() -> void {
    enabled_ = false;
  }

 protected:
  /// @brief Copied, moved and destroyed only as part of the derived class.
  ///
  /// Protected rather than public so that the base cannot be made, copied or destroyed on its
  /// own - which would slice a derived node down to it (C.35, C.67). The constructors stay public:
  /// a derived class takes them with `using base::base;`, and an inherited constructor keeps the
  /// base's access, so a protected one would make every derived node unconstructible.
  constexpr node(const node &) = default;
  constexpr node(node &&) = default;
  constexpr auto operator=(const node &) -> node & = default;
  constexpr auto operator=(node &&) -> node & = default;
  constexpr ~node() = default;

 private:
  /// @brief A delta with every dimension set to the same value.
  static constexpr auto filled(value_type value) -> point {
    point out{};
    for (std::size_t i = 0; i < dimension; ++i) {
      out[i] = value;
    }
    return out;
  }

  estimation_type estimation_{};
  point sigma_{};
  bool enabled_{true};
};

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_NODE_HPP
