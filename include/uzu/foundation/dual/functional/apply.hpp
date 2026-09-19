/// ===============================================================================================
/// @file
///
/// @brief Provides generic 'apply' utilities for invoking predicates/functions
/// on dual arrays, std::arrays, std::tuples, and other types.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_APPLY_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_APPLY_HPP

#include <functional>

#include "uzu/foundation/dual/array.hpp"

namespace dual {

/// @brief Applies a predicate to a std::array using std::apply.
/// @tparam F Type of predicate/function to apply.
/// @tparam T Type of array element.
/// @tparam N Size of the array.
/// @param predicate The function or callable to apply.
/// @param data The std::array to unpack and apply the predicate to.
/// @return Result of invoking predicate with unpacked array elements.
template <class F, class T, std::size_t N>
inline constexpr auto apply(F predicate, const std::array<T, N> &data) {
  return std::apply(predicate, data);
}

/// @brief Applies a predicate to a dual::array by unpacking its storage.
/// @tparam F Type of predicate/function to apply.
/// @tparam Ts Types of array elements.
/// @param predicate The function or callable to apply.
/// @param data The dual::array to unpack and apply the predicate to.
/// @return Result of invoking predicate with unpacked array elements.
template <class F, class... Ts>
inline constexpr auto apply(F predicate, const array<Ts...> &data) {
  return std::apply(predicate, data.storage());
}

/// @brief Applies a predicate to a std::tuple using std::apply.
/// @tparam F Type of predicate/function to apply.
/// @tparam Ts Types of tuple elements.
/// @param predicate The function or callable to apply.
/// @param data The std::tuple to unpack and apply the predicate to.
/// @return Result of invoking predicate with unpacked tuple elements.
template <class F, class... Ts>
inline constexpr auto apply(F predicate, const std::tuple<Ts...> &data) {
  return std::apply(predicate, data);
}

/// @brief Applies a predicate to a single value using std::invoke.
/// @tparam F Type of predicate/function to apply.
/// @tparam T Type of value.
/// @param predicate The function or callable to apply.
/// @param data The value to apply the predicate to.
/// @return Result of invoking predicate with the value.
template <class F, class T>
inline constexpr auto apply(F predicate, const T &data) {
  return std::invoke(predicate, data);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_APPLY_HPP
