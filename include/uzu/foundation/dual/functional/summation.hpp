/// ===============================================================================================
/// @file
///
/// @brief Provides utilities to compute the sum of applying a predicate to each
/// element of a tuple or dual::array.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_SUMMATION_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_SUMMATION_HPP

#include "uzu/foundation/dual/array.hpp"

namespace dual {

/// @brief Computes the sum of applying a predicate to each element of a
/// std::tuple.
/// @tparam F Type of the predicate/function to apply.
/// @tparam Ts Types of the tuple elements.
/// @param data The std::tuple whose elements will be processed.
/// @param predicate The function to apply to each element before summing.
/// @return The sum of the results of predicate applied to each tuple element.
template <class F, class... Ts>
inline constexpr auto summation(const std::tuple<Ts...> &data, F predicate) {
  return apply([&](const auto &...elems) { return (apply(predicate, elems) + ...); }, data);
}

/// @brief Computes the sum of applying a predicate to each element of a
/// dual::array.
/// @tparam F Type of the predicate/function to apply.
/// @tparam Ts Types of the array elements.
/// @param data The dual::array whose elements will be processed.
/// @param predicate The function to apply to each element before summing.
/// @return The sum of the results of predicate applied to each array element.
template <class F, class... Ts>
inline constexpr auto summation(const array<Ts...> &data, F predicate) {
  return apply(
      [&](const auto &...elems) { return (apply(predicate, elems) + ...); }, data.storage());
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_SUMMATION_HPP
