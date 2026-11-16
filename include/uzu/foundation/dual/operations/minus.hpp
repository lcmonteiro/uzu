/// ===============================================================================================
/// @file
///
/// @brief Defines subtraction operations for dual numbers, including derivative
/// computations, element-wise array subtraction, and broadcasting.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_MINUS_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_MINUS_HPP

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Binary functor for subtraction and its derivatives for dual numbers.
struct minus : binary_operation<minus> {
  /// @brief Computes the subtraction of two values.
  /// @tparam T The numeric type.
  /// @param v1 Minuend.
  /// @param v2 Subtrahend.
  /// @return The result of v1 - v2.
  template <class T>
  constexpr auto value(const T &v1, const T &v2) const {
    return v1 - v2;
  }

  /// @brief Computes the derivative for subtraction of two dual numbers.
  /// @tparam T Numeric type.
  /// @param n1 Minuend dual value and derivative.
  /// @param n2 Subtrahend dual value and derivative.
  /// @return The derivative of n1.v - n2.v.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const duo<T> &n2) const {
    return n1.d - n2.d;
  }

  /// @brief Computes the derivative for subtraction where only the subtrahend
  /// is dual.
  /// @tparam T Numeric type.
  /// @param v1 Minuend value.
  /// @param n2 Subtrahend dual value and derivative.
  /// @return The derivative of v1 - n2.v.
  template <class T>
  constexpr auto dvalue(const T &, const duo<T> &n2) const {
    return -n2.d;
  }

  /// @brief Computes the derivative for subtraction where only the minuend is
  /// dual.
  /// @tparam T Numeric type.
  /// @param n1 Minuend dual value and derivative.
  /// @param v2 Subtrahend value.
  /// @return The derivative of n1.v - v2.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const T &) const {
    return n1.d;
  }
};

/// @brief Overloads the subtraction operator for dual numbers and scalars.
/// @tparam T1 Minuend type.
/// @tparam T2 Subtrahend type.
/// @param n1 Minuend.
/// @param n2 Subtrahend.
/// @return Result of subtraction using dual arithmetic.
template <class T1, class T2>
  requires number_operands<T1, T2>
inline constexpr auto operator-(const T1 &n1, const T2 &n2) {
  return minus{}(n1, n2);
}

/// @brief `-` element by element: two arrays of the same length, or an array and one value
/// applied to every element. See `elementwise`.
template <class T1, class T2>
  requires elementwise_operands<T1, T2>
inline constexpr auto operator-(const T1 &c1, const T2 &c2) {
  return elementwise<minus>{}(c1, c2);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_MINUS_HPP
