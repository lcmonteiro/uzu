/// ===============================================================================================
/// @file
///
/// @brief Plain gradient descent.
///
/// See `uzu/optimization/algorithm.hpp` for what an algorithm has to supply and why it is two
/// types.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_ALGORITHM_GRADIENT_HPP
#define UZU_OPTIMIZATION_ALGORITHM_GRADIENT_HPP

#include <cstddef>

#include "uzu/helpers/keywords.hpp"

namespace uzu {

/// @brief Plain gradient descent: the step is the bounded gradient, scaled.
///
/// Keeps nothing, so it is its own instance at every width and the index goes unused.
class gradient {
 public:
  using value_type = double;

  /// @brief What a graph of this width runs. Stateless, so the builder itself.
  template <std::size_t Width>
  using instance = gradient;

  /// @brief Builds the algorithm from `lr`, which defaults to 0.1.
  template <impl::keyword_argument... Args>
  explicit constexpr gradient(const Args &...args)
      : rate_{impl::find_one<impl::lr_tag>(value_type{0.1}, args...)} {
  }

  /// @brief Nothing to size, so the instance is a copy of the builder.
  template <std::size_t Width>
  constexpr auto build() const -> instance<Width> {
    return *this;
  }

  /// @brief The delta for one bounded gradient component.
  template <std::size_t Index>
  constexpr auto step(value_type gradient) -> value_type {
    return -rate_ * gradient;
  }

 private:
  value_type rate_{};
};

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_ALGORITHM_GRADIENT_HPP
