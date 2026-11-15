/// ===============================================================================================
/// @file
///
/// @brief Defines square root operations for dual numbers, including derivative
/// computations and element-wise array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_SQRT_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_SQRT_HPP

#include <cmath>
#include <functional>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing square root and its derivative for dual
/// numbers.
struct sqrt_operation : unary_operation<sqrt_operation> {
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
/// @brief `sqrt` for dual numbers. Found by ADL: call it unqualified.
/// @tparam T The numeric type.
/// @param n The dual number.
/// @return The result of applying sqrt using dual arithmetic.
template <number_operand T>
inline auto sqrt(const T &n) {
  return std::invoke(sqrt_operation{}, n);
}
/// @brief Functor for element-wise square root transformation on arrays.
struct sqrt_transform : transform_unary_operation<sqrt_transform> {
  /// @brief Applies square root to an array element.
  /// @tparam T The numeric type of the element.
  /// @param n The array element.
  /// @return The square root of the element.
  template <class T>
  auto transform(const T &n) const {
    return sqrt(n);
  }
};
/// @brief `sqrt` for arrays of dual numbers. Found by ADL: call it unqualified.
/// @tparam T The array type.
/// @param n The array of dual numbers.
/// @return An array with each element as the square root of the original
/// element.
template <array_operand T>
inline auto sqrt(const T &n) {
  return std::invoke(sqrt_transform{}, n);
}
}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_SQRT_HPP
