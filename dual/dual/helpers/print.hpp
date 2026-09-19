/// @file print.hpp
/// @brief Provides print utilities for dual numbers, dual arrays, and standard
/// containers. Functions are overloaded to support various dual types and
/// variadic printing.

#pragma once

#include <array>
#include <iostream>

#include "dual/array.hpp"
#include "dual/number.hpp"

namespace dual {

/// @brief Prints a dual::number, showing its value and the derivatives for each
/// index.
/// @tparam U Underlying numeric type.
/// @tparam Is Indices for derivatives.
/// @param n The dual number to print.
template <class U, size_t... Is>
inline auto print(const number<U, Is...> &n);

/// @brief Prints a dual::array, recursively printing each element.
/// @tparam U Types of array elements.
/// @param data The dual array to print.
template <class... U>
inline auto print(const array<U...> &data);

/// @brief Prints a single value (generic overload, typically for scalars).
/// @tparam U Type of the value.
/// @param value The value to print.
template <class U>
inline auto print(const U &value);

/// @brief Prints multiple values, each on a separate line.
/// @tparam U Type of first value.
/// @tparam Us Types of subsequent values.
/// @param value First value to print.
/// @param values Remaining values to print.
template <class U, class... Us>
inline auto print(const U &value, const Us &...values);

/// @brief Prints a std::array by recursively printing its elements.
/// @tparam U Type of array elements.
/// @tparam N Size of the array.
/// @param data The std::array to print.
template <class U, size_t N>
inline auto print(const std::array<U, N> &data);

/// @brief Implementation: Prints a dual::number, showing value and all
/// derivatives.
template <class U, size_t... Is>
inline auto print(const number<U, Is...> &n) {
  std::cout << n.value() << " [";
  ((std::cout << Is << ":" << n.template dvalue<Is>() << " "), ...);
  std::cout << "]" << std::endl;
}

/// @brief Implementation: Prints a dual::array by applying print to all storage
/// elements.
template <class... U>
inline auto print(const array<U...> &data) {
  std::apply([](auto &...v) { print(v...); }, data.storage());
}

/// @brief Implementation: Prints a std::array by applying print to each
/// element.
template <class U, size_t N>
inline auto print(const std::array<U, N> &data) {
  std::apply([](auto &...v) { print(v...); }, data);
}

/// @brief Implementation: Prints a single value to std::cout.
template <class U>
inline auto print(const U &value) {
  std::cout << value << std::endl;
}

/// @brief Implementation: Prints multiple values, each on its own line.
template <class U, class... Us>
inline auto print(const U &value, const Us &...values) {
  print(value);
  (print(values), ...);
}

}  // namespace dual
