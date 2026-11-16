/// ===============================================================================================
/// @file
///
/// @brief Provides print utilities for dual numbers, dual arrays, and standard
/// containers. Functions are overloaded to support various dual types and
/// variadic printing.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_PRINT_HPP
#define UZU_FOUNDATION_DUAL_PRINT_HPP

#include <array>
#include <iostream>

#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/number.hpp"

namespace dual {

/// @brief Prints a dual::number, showing its value and the derivatives for each
/// index.
template <class U, size_t... Is>
inline auto print(const number<U, Is...> &n);

/// @brief Prints a dual::array, recursively printing each element.
template <class... U>
inline auto print(const array<U...> &data);

/// @brief Prints a single value (generic overload, typically for scalars).
template <class U>
inline auto print(const U &value);

/// @brief Prints multiple values, each on a separate line.
template <class U, class... Us>
inline auto print(const U &value, const Us &...values);

/// @brief Prints a std::array by recursively printing its elements.
template <class U, size_t N>
inline auto print(const std::array<U, N> &data);

template <class U, size_t... Is>
inline auto print(const number<U, Is...> &n) {
  std::cout << n.value() << " [";
  ((std::cout << Is << ":" << n.template dvalue<Is>() << " "), ...);
  std::cout << "]" << std::endl;
}

template <class... U>
inline auto print(const array<U...> &data) {
  std::apply([](auto &...v) { print(v...); }, data.storage());
}

template <class U, size_t N>
inline auto print(const std::array<U, N> &data) {
  std::apply([](auto &...v) { print(v...); }, data);
}

template <class U>
inline auto print(const U &value) {
  std::cout << value << std::endl;
}

template <class U, class... Us>
inline auto print(const U &value, const Us &...values) {
  print(value);
  (print(values), ...);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_PRINT_HPP
