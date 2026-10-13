/// ===============================================================================================
/// @file
///
/// @brief Resilient propagation: a step size per index, adapted by the sign of the gradient.
///
/// See `uzu/optimization/algorithm.hpp` for what an algorithm has to supply and why it is two
/// types.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_ALGORITHM_RPROP_HPP
#define UZU_OPTIMIZATION_ALGORITHM_RPROP_HPP

#include <algorithm>
#include <array>
#include <cstddef>

#include "uzu/helpers/keywords.hpp"

namespace uzu {

/// @brief Resilient propagation (iRprop-, Igel and Huesken's variant of Riedmiller and Braun's
/// rule): every index keeps its own step size, and only the sign of the gradient is used.
///
///     same sign as last pass     step <- min(step * 1.2, step_max),  delta = -sign(g) * step
///     sign flipped               step <- max(step * 0.5, step_min),  delta = 0, forget the sign
///     zero                       delta = 0
///
/// It suits the graph's kernel better than either descent rule does. The kernel hands the
/// algorithm `sign(g) * radial(g, sigma)`: saturated at one far from the minimum, where a fixed
/// `lr` turns into a fixed step that overshoots, and shrinking with the square of the gradient
/// near it, where a fixed `lr` crawls. Rprop reads neither magnitude. A step grows geometrically
/// while the slope keeps its sign, so a far-off estimate covers ground in a few dozen passes,
/// and halves every time it is overshot, so it closes in geometrically however flat the kernel
/// has made the slope.
///
/// It is also why `lr` means something else here: it is the step every index *starts* with, in
/// the estimate's own units, not a factor on the gradient. Rprop is not sensitive to it - a step
/// that starts ten times too small is grown back in a dozen passes - which is the other reason
/// to use it: there is no rate to sweep for.
///
/// The rule is meant for exact gradients, which is what the graph computes: there is no sampling
/// noise for a sign to be fooled by.
///
/// What stops it is the node's sigma, not `step_min`. The kernel's output is exactly zero once
/// `|g| < 4.2e-8 * sigma` (below that, `radial` rounds `1 - g^2 / 2 sigma^2 N` to one), and a zero
/// takes no step. So the node sigma is not a step-size knob here, but it is the convergence
/// threshold: a smaller sigma fits further before an index goes quiet.
class rprop {
 public:
  using value_type = double;

  /// @brief How much a step grows while the sign holds, and shrinks when it flips. The values
  /// are the ones the rule was published with. Their product has to stay below one, or a pair of
  /// indices flipping sign every other pass never settles: at 2.0 and 0.5 no fit converged at all.
  /// Between 1.1 and 1.5, and 0.5 and 0.7, the passes a fit needs move by under 2x either way.
  static constexpr value_type grow = 1.2;
  static constexpr value_type shrink = 0.5;
  static_assert(grow > 1 && shrink < 1 && grow * shrink < 1, "an oscillating index must settle");

  /// @brief The bounds on a step, in the estimate's own units.
  ///
  /// The upper one keeps a run of agreeing signs from compounding without limit - which matters
  /// more here than elsewhere, because a step that throws a node past the kernel's rejection
  /// point of every edge it has leaves it with no gradient at all, and nothing ever moves it
  /// back. Fits that start tens of units away peak around ten; this is a guard, not a pace.
  ///
  /// The lower one is set below anything a fit reaches, so that the kernel's dead zone stops a
  /// fit rather than this does (see above). Fits end on steps of 1e-10 to 1e-8. At the 1e-6 the
  /// rule is usually published with, every index would instead oscillate at 1e-6 forever.
  static constexpr value_type step_max = 50.0;
  static constexpr value_type step_min = 1e-12;

  /// @brief Rprop over a graph of known width: a step size and the last sign, per index.
  template <std::size_t Width>
  class instance {
   public:
    using value_type = double;

    explicit constexpr instance(value_type initial) {
      for (auto &s : step_) {
        s = initial;
      }
    }

    /// @brief The delta for index @p Index, from the sign of its bounded gradient.
    template <std::size_t Index>
    constexpr auto step(value_type gradient) -> value_type {
      static_assert(Index < Width, "index outside the graph this instance was built for");

      auto &size = step_[Index];
      auto &last = last_[Index];

      const auto sign = gradient > zero ? one : gradient < zero ? -one : zero;
      const auto sign_product = sign * last;
      if (sign_product < zero) {
        // Overshot: shrink the step and skip this iteration. The sign is forgotten too, so the
        // next pass neither grows nor shrinks again on the strength of the one just undone.
        size = std::max(step_min, size * shrink);
        last = zero;
        return zero;
      }

      if (sign_product > zero) {
        // Consistent direction: grow the step.
        size = std::min(step_max, size * grow);
      }

      last = sign;
      return -sign * size;
    }

   private:
    static constexpr value_type zero = 0.0;
    static constexpr value_type one = 1.0;

    std::array<value_type, Width> step_{};
    std::array<value_type, Width> last_{};
  };

  /// @brief Builds the algorithm from `lr`, the step every index starts with. Defaults to 0.1.
  template <impl::keyword_argument... Args>
  explicit constexpr rprop(const Args &...args)
      : initial_{impl::find_one<impl::lr_tag>(value_type{0.1}, args...)} {
  }

  /// @brief An instance at rest: every step at its initial size, no sign seen yet.
  template <std::size_t Width>
  constexpr auto build() const -> instance<Width> {
    return instance<Width>{initial_};
  }

 private:
  value_type initial_{};
};

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_ALGORITHM_RPROP_HPP
