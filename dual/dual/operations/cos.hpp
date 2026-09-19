/// @file cos.hpp
/// @brief Defines cosine operations for dual numbers, including derivative
/// computations and array transforms.

#pragma once

#include <cmath>
#include <functional>

#include "dual/helpers/operations.hpp"

namespace dual {
/// @brief Functor for computing the cosine and its derivative for dual numbers.
struct cos : unary_operation<cos> {
  /// @brief Computes the cosine of a value.
  /// @tparam T The numeric type of the value.
  /// @param v The input value.
  /// @return The cosine of v.
  template <class T>
  auto value(const T &v) const {
    return std::cos(v);
  }

  /// @brief Computes the derivative of the cosine function.
  /// @tparam T The numeric type of the value.
  /// @param n A duo containing the input value and its derivative.
  /// @return The derivative of cos(n.v), which is -sin(n.v) * n.d.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return -std::sin(n.v) * n.d;
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the standard cos function for dual numbers.
/// @tparam T The numeric type.
/// @param n The dual number.
/// @return The cosine of the dual number.
template <dual::number_operand T>
inline auto cos(const T &n) {
  return std::invoke(dual::cos{}, n);
}
}  // namespace std

namespace dual {
/// @brief Functor for element-wise cosine transformation on dual arrays.
struct cos_transform : transform_unary_operation<cos_transform> {
  /// @brief Applies the cosine transformation to an array element.
  /// @tparam T The numeric type of the element.
  /// @param n The element.
  /// @return The cosine of the element.
  template <class T>
  auto transform(const T &n) const {
    return std::cos(n);
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the standard cos function for arrays of dual numbers.
/// @tparam T The array type.
/// @param n The array of dual numbers.
/// @return An array of cosines of the dual numbers.
template <dual::array_operand T>
inline auto cos(const T &n) {
  return std::invoke(dual::cos_transform{}, n);
}
}  // namespace std
