/// ===============================================================================================
/// @file
///
/// @brief Provides generic 'apply' utilities for invoking functions
/// on dual arrays, std::arrays, std::tuples, and other types.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_APPLY_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_APPLY_HPP

#include <functional>

#include "uzu/foundation/dual/array.hpp"

namespace dual {

/// @brief Applies a function to a std::array using std::apply.
template <class F, class T, std::size_t N>
inline constexpr auto apply(F f, const std::array<T, N> &data) {
  return std::apply(f, data);
}

/// @brief Applies a function to a dual::array by unpacking its storage.
template <class F, class... Ts>
inline constexpr auto apply(F f, const array<Ts...> &data) {
  return std::apply(f, data.storage());
}

/// @brief Applies a function to a std::tuple using std::apply.
template <class F, class... Ts>
inline constexpr auto apply(F f, const std::tuple<Ts...> &data) {
  return std::apply(f, data);
}

/// @brief Applies a function to a single value using std::invoke.
template <class F, class T>
inline constexpr auto apply(F f, const T &data) {
  return std::invoke(f, data);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_APPLY_HPP
