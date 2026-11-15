/// ===============================================================================================
/// @file
///
/// @brief Defines sine operations for dual numbers, including derivative
/// computations and element-wise array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_SIN_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_SIN_HPP

#include <cmath>
#include <functional>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing sine and its derivative for dual numbers.
struct sin_operation : unary_operation<sin_operation> {
  /// @brief Computes the sine of a value.
  /// @tparam T The numeric type of the value.
  /// @param v The input value.
  /// @return The sine of v.
  template <class T>
  auto value(const T &v) const {
    return std::sin(v);
  }

  /// @brief Computes the derivative of the sine operation.
  /// @tparam T The numeric type of the value.
  /// @param n A duo containing the input value and its derivative.
  /// @return The derivative of sin(n.v).
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return std::cos(n.v) * n.d;
  }
};
/// @brief `sin` for dual numbers. Found by ADL: call it unqualified.
/// @tparam T The numeric type.
/// @param n The dual number.
/// @return The result of applying sine using dual arithmetic.
template <number_operand T>
inline auto sin(const T &n) {
  return std::invoke(sin_operation{}, n);
}
/// @brief Functor for element-wise sine transformation on arrays.
struct sin_transform : transform_unary_operation<sin_transform> {
  /// @brief Applies sine to an array element.
  /// @tparam T The numeric type of the element.
  /// @param n The array element.
  /// @return The sine of the element.
  template <class T>
  auto transform(const T &n) const {
    return sin(n);
  }
};
/// @brief `sin` for arrays of dual numbers. Found by ADL: call it unqualified.
/// @tparam T The array type.
/// @param n The array of dual numbers.
/// @return An array with each element as the sine of the original element.
template <array_operand T>
inline auto sin(const T &n) {
  return std::invoke(sin_transform{}, n);
}
}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_SIN_HPP
