/// ===============================================================================================
/// @file
///
/// @brief Provides utilities to transform elements of dual::array and
/// std::array using a predicate.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_TRANSFORM_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_TRANSFORM_HPP

#include "uzu/foundation/dual/array.hpp"

namespace dual {

/// @brief Applies a predicate to each element of a dual::array, returning a new
/// dual::array.
/// @tparam F Type of the predicate/function to apply.
/// @tparam Ts Types of the array elements.
/// @param data The dual::array whose elements will be transformed.
/// @param predicate The function to apply to each element.
/// @return A new dual::array with transformed elements.
template <class F, class... Ts>
inline auto transform(const array<Ts...> &data, F predicate) {
  return std::apply(
      [&](const auto &...elems) { return array(std::apply(predicate, elems)...); }, data.storage());
}

/// @brief Applies a predicate to each element of a std::array, returning a new
/// std::array.
/// @tparam F Type of the predicate/function to apply.
/// @tparam T Type of the array elements.
/// @tparam N Size of the array.
/// @param data The std::array whose elements will be transformed.
/// @param predicate The function to apply to each element.
/// @return A new std::array with transformed elements.
template <class F, class T, std::size_t N>
inline auto transform(const std::array<T, N> &data, F predicate) {
  return std::apply([&](const auto &...elems) { return std::array{predicate(elems)...}; }, data);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_TRANSFORM_HPP
