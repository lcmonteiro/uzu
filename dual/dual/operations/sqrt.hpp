/// @file sqrt.hpp
/// @brief Defines square root operations for dual numbers, including derivative
/// computations and element-wise array transforms.

#pragma once

#include <cmath>
#include <functional>

#include "dual/helpers/operations.hpp"

namespace dual {
/// @brief Functor for computing square root and its derivative for dual
/// numbers.
struct sqrt : unary_operation<sqrt> {
  /// @brief Computes the square root of a value.
  /// @tparam T The numeric type of the value.
  /// @param v The input value.
  /// @return The square root of v.
  template <class T>
  auto value(const T &v) const {
    return std::sqrt(v);
  }

  /// @brief Computes the derivative of the square root operation.
  /// @tparam T The numeric type of the value.
  /// @param n A duo containing the input value and its derivative.
  /// @return The derivative of sqrt(n.v).
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return n.d / (static_cast<T>(2.0) * std::sqrt(n.v));
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the sqrt function for dual numbers.
/// @tparam T The numeric type.
/// @param n The dual number.
/// @return The result of applying sqrt using dual arithmetic.
template <dual::number_operand T>
inline auto sqrt(const T &n) {
  return std::invoke(dual::sqrt{}, n);
}
}  // namespace std

namespace dual {
/// @brief Functor for element-wise square root transformation on arrays.
struct sqrt_transform : transform_unary_operation<sqrt_transform> {
  /// @brief Applies square root to an array element.
  /// @tparam T The numeric type of the element.
  /// @param n The array element.
  /// @return The square root of the element.
  template <class T>
  auto transform(const T &n) const {
    return std::sqrt(n);
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the sqrt function for arrays of dual numbers.
/// @tparam T The array type.
/// @param n The array of dual numbers.
/// @return An array with each element as the square root of the original
/// element.
template <dual::array_operand T>
inline auto sqrt(const T &n) {
  return std::invoke(dual::sqrt_transform{}, n);
}
}  // namespace std
