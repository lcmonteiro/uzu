/// ===============================================================================================
/// @file
///
/// @brief Provides utilities to compute the sum of applying a function to each
/// element of a tuple or dual::array.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_SUMMATION_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_SUMMATION_HPP

#include "uzu/foundation/dual/array.hpp"

namespace dual {

/// @brief Computes the sum of applying a function to each element of a
/// std::tuple.
template <class F, class... Ts>
inline constexpr auto summation(const std::tuple<Ts...> &data, F f) {
  return apply([&](const auto &...elems) { return (apply(f, elems) + ...); }, data);
}

/// @brief Computes the sum of applying a function to each element of a
/// dual::array.
template <class F, class... Ts>
inline constexpr auto summation(const array<Ts...> &data, F f) {
  return apply([&](const auto &...elems) { return (apply(f, elems) + ...); }, data.storage());
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_SUMMATION_HPP
