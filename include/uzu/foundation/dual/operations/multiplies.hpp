/// ===============================================================================================
/// @file
///
/// @brief Defines multiplication operations for dual numbers, including
/// derivative computations, element-wise array multiplication, and
/// broadcasting.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_MULTIPLIES_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_MULTIPLIES_HPP

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Binary functor for multiplication and its derivatives for dual
/// numbers.
struct multiplies : binary_operation<multiplies> {
  /// @brief Computes the multiplication of two values.
  /// @tparam T The numeric type.
  /// @param v1 The first value (left operand).
  /// @param v2 The second value (right operand).
  /// @return The result of v1 * v2.
  template <class T>
  constexpr auto value(const T &v1, const T &v2) const {
    return v1 * v2;
  }

  /// @brief Computes the derivative of the multiplication for two dual numbers.
  /// @tparam T Numeric type.
  /// @param n1 The first dual value and derivative.
  /// @param n2 The second dual value and derivative.
  /// @return The derivative of n1.v * n2.v.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const duo<T> &n2) const {
    return n1.v * n2.d + n2.v * n1.d;
  }

  /// @brief Computes the derivative of the multiplication for a scalar and a
  /// dual number.
  /// @tparam T Numeric type.
  /// @param v1 The scalar value.
  /// @param n2 The dual value and derivative.
  /// @return The derivative of v1 * n2.v.
  template <class T>
  constexpr auto dvalue(const T &v1, const duo<T> &n2) const {
    return v1 * n2.d;
  }

  /// @brief Computes the derivative of the multiplication for a dual number and
  /// a scalar.
  /// @tparam T Numeric type.
  /// @param n1 The dual value and derivative.
  /// @param v2 The scalar value.
  /// @return The derivative of n1.v * v2.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const T &v2) const {
    return v2 * n1.d;
  }
};

/// @brief Overloads the multiplication operator for dual numbers and scalars.
/// @tparam T1 First operand type.
/// @tparam T2 Second operand type.
/// @param n1 First operand.
/// @param n2 Second operand.
/// @return Result of multiplication using dual arithmetic.
template <class T1, class T2>
  requires number_operands<T1, T2>
inline constexpr auto operator*(const T1 &n1, const T2 &n2) {
  return multiplies{}(n1, n2);
}

/// @brief `*` element by element: two arrays of the same length, or an array and one value
/// applied to every element. See `elementwise`.
template <class T1, class T2>
  requires elementwise_operands<T1, T2>
inline constexpr auto operator*(const T1 &c1, const T2 &c2) {
  return elementwise<multiplies>{}(c1, c2);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_MULTIPLIES_HPP
