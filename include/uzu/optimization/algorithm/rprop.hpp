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
class rprop {
 public:
  using value_type = double;

  /// @brief How much a step grows while the sign holds, and shrinks when it flips. The values
  /// are the ones the rule was published with, and the ones it is usually run with.
  static constexpr value_type grow = 1.2;
  static constexpr value_type shrink = 0.5;

  /// @brief The bounds on a step. The upper one keeps a run of agreeing signs from compounding
  /// without limit; the lower one is far enough down not to be what stops a fit.
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

      const auto sign = gradient > 0 ? 1 : gradient < 0 ? -1 : 0;
      if (sign * last > 0) {
        size = size * grow < step_max ? size * grow : step_max;
      } else if (sign * last < 0) {
        // Overshot: shrink, take no step, and forget the sign so the next pass neither grows
        // nor shrinks again on the strength of the one that was just undone.
        size = size * shrink > step_min ? size * shrink : step_min;
        last = 0;
        return 0;
      }
      last = sign;
      return -static_cast<value_type>(sign) * size;
    }

   private:
    std::array<value_type, Width> step_{};
    std::array<int, Width> last_{};
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
