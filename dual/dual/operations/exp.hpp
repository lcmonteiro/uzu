/// @file exp.hpp
/// @brief Defines exponential operations for dual numbers, including derivative
/// computations and element-wise array transforms.
///
/// This is the one operation that cannot run in a constant expression: it bottoms out in
/// `std::exp`, and none of `<cmath>` is `constexpr` before C++26. An expression that reaches for
/// it is a run-time expression, whatever the functions around it are marked.

#pragma once

#include <cmath>
#include <functional>

#include "dual/helpers/operations.hpp"

namespace dual {
/// @brief Functor for computing the exponential function and its derivative for
/// dual numbers.
struct exp : unary_operation<exp> {
  /// @brief Computes the exponential of a value.
  /// @tparam T The numeric type of the value.
  /// @param v The input value.
  /// @return The exponential of v.
  template <class T>
  auto value(const T &v) const {
    return std::exp(v);
  }

  /// @brief Computes the derivative of the exponential function.
  /// @tparam T The numeric type of the value.
  /// @param n A duo containing the input value and its derivative.
  /// @return The derivative of exp(n.v), which is exp(n.v) * n.d.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return std::exp(n.v) * n.d;
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the standard exp function for dual numbers.
/// @tparam T The numeric type.
/// @param n The dual number.
/// @return The exponential of the dual number.
template <dual::number_operand T>
inline auto exp(const T &n) {
  return std::invoke(dual::exp{}, n);
}
}  // namespace std

namespace dual {
/// @brief Functor for element-wise exponential transformation on dual arrays.
struct exp_transform : transform_unary_operation<exp_transform> {
  /// @brief Applies the exponential transformation to an array element.
  /// @tparam T The numeric type of the element.
  /// @param n The element.
  /// @return The exponential of the element.
  template <class T>
  auto transform(const T &n) const {
    return std::exp(n);
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the standard exp function for arrays of dual numbers.
/// @tparam T The array type.
/// @param n The array of dual numbers.
/// @return An array of exponentials of the dual numbers.
template <dual::array_operand T>
inline auto exp(const T &n) {
  return std::invoke(dual::exp_transform{}, n);
}
}  // namespace std
