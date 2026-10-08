/// ===============================================================================================
/// @file
///
/// @brief Gradient descent with momentum.
///
/// See `uzu/optimization/graph_algorithm.hpp` for what an algorithm has to supply and why it is two
/// types.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_ALGORITHMS_MOMENTUM_HPP
#define UZU_OPTIMIZATION_GRAPH_ALGORITHMS_MOMENTUM_HPP

#include <array>
#include <cstddef>

#include "uzu/helpers/keywords.hpp"

namespace uzu {

/// @brief Gradient descent with momentum: the heavy ball.
///
/// Each index carries a velocity that the bounded gradient accelerates and `beta` bleeds away:
///
///     v <- beta * v + g
///     delta = -lr * v
///
/// Along a direction the gradient keeps pointing the same way, the velocity settles at
/// `g / (1 - beta)`, so the step there is up to `1 / (1 - beta)` times the plain one - which is
/// the whole point, and also the catch. The graph's kernel bounds the *gradient* to one, not the
/// step, so with `beta` at 0.9 the step can reach ten times `lr`. Momentum trades the plain rule's
/// hard bound for speed along a consistent slope, and a momentum `lr` is not comparable to a
/// descent `lr` of the same number.
///
/// `beta = 0` leaves `v = g` and the rule is exactly plain descent, which is what the tests hold
/// it to.
class momentum {
 public:
  using value_type = double;

  /// @brief Momentum over a graph of known width: the velocities included.
  ///
  /// This is what the graph holds, and what the builder exists to produce. It is a separate type
  /// only so that the velocity can be a `std::array` sized by the width, which is not known where
  /// the builder is written.
  template <std::size_t Width>
  class instance {
   public:
    using value_type = double;

    constexpr instance(value_type rate, value_type decay) : rate_{rate}, decay_{decay} {
    }

    /// @brief The delta for one bounded gradient component.
    template <std::size_t Index>
    constexpr auto step(value_type gradient) -> value_type {
      static_assert(Index < Width, "index outside the graph this instance was built for");

      auto &v = velocity_[Index];

      v = decay_ * v + gradient;

      return -rate_ * v;
    }

   private:
    value_type rate_{};
    value_type decay_{};
    std::array<value_type, Width> velocity_{};
  };

  /// @brief Builds the algorithm from `lr` and `beta`, defaulting to 0.1 and 0.9.
  template <impl::keyword_argument... Args>
  explicit constexpr momentum(const Args &...args)
      : rate_{impl::find_one<impl::lr_tag>(value_type{0.1}, args...)},
        decay_{impl::find_one<impl::beta_tag>(value_type{0.9}, args...)} {
  }

  /// @brief The instance a graph of this width runs, with every velocity at rest.
  template <std::size_t Width>
  constexpr auto build() const -> instance<Width> {
    return instance<Width>{rate_, decay_};
  }

 private:
  value_type rate_{};
  value_type decay_{};
};

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_ALGORITHMS_MOMENTUM_HPP
