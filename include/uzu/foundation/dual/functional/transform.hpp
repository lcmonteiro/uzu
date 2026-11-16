/// ===============================================================================================
/// @file
///
/// @brief `transform`: a new array from applying a function to each element of one.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_TRANSFORM_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_TRANSFORM_HPP

#include <array>
#include <cstddef>
#include <tuple>

#include "uzu/foundation/dual/array.hpp"

namespace dual {

/// @brief Applies @p f to each element of a dual::array, returning a new dual::array.
/// @tparam F Callable taking one element and returning a `dual::number`.
/// @tparam Ts Types of the array elements.
/// @param data The array whose elements will be transformed.
/// @param f The function to apply to each element.
/// @return A dual::array of what @p f returned, in order.
template <class F, class... Ts>
constexpr auto transform(const array<Ts...> &data, F f) {
  return std::apply([&](const auto &...elems) { return array(f(elems)...); }, data.storage());
}

/// @brief Applies @p f to each element of a std::array, returning a new std::array.
/// @tparam F Callable taking one element.
/// @tparam T Type of the array elements.
/// @tparam N Size of the array.
/// @param data The array whose elements will be transformed.
/// @param f The function to apply to each element.
/// @return A std::array of what @p f returned, in order.
template <class F, class T, std::size_t N>
constexpr auto transform(const std::array<T, N> &data, F f) {
  return std::apply([&](const auto &...elems) { return std::array{f(elems)...}; }, data);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_TRANSFORM_HPP
