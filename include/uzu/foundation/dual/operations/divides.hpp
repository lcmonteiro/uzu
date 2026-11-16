/// ===============================================================================================
/// @file
///
/// @brief Defines division operations for dual numbers, including derivative
/// computations,
///        element-wise array division, and broadcasting.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_DIVIDES_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_DIVIDES_HPP

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Binary functor for division and its derivatives for dual numbers.
struct divides : binary_operation<divides> {
  /// @brief Computes the division of two values.
  /// @tparam T The numeric type.
  /// @param v1 Numerator.
  /// @param v2 Denominator.
  /// @return The result of v1 / v2.
  template <class T>
  constexpr auto value(const T &v1, const T &v2) const {
    return v1 / v2;
  }

  /// @brief Computes the derivative of the division for two dual numbers.
  /// @tparam T Numeric type.
  /// @param n1 Numerator dual value and derivative.
  /// @param n2 Denominator dual value and derivative.
  /// @return The derivative of n1.v / n2.v.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const duo<T> &n2) const {
    return (n2.v * n1.d - n1.v * n2.d) / (n2.v * n2.v);
  }

  /// @brief Computes the derivative of the division for a scalar numerator and
  /// dual denominator.
  /// @tparam T Numeric type.
  /// @param v1 Numerator value.
  /// @param n2 Denominator dual value and derivative.
  /// @return The derivative of v1 / n2.v.
  template <class T>
  constexpr auto dvalue(const T &v1, const duo<T> &n2) const {
    return (-v1 * n2.d) / (n2.v * n2.v);
  }

  /// @brief Computes the derivative of the division for a dual numerator and
  /// scalar denominator.
  /// @tparam T Numeric type.
  /// @param n1 Numerator dual value and derivative.
  /// @param v2 Denominator value.
  /// @return The derivative of n1.v / v2.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const T &v2) const {
    return (v2 * n1.d) / (v2 * v2);
  }
};

/// @brief Overloads the division operator for dual numbers and scalars.
/// @tparam T1 Numerator type.
/// @tparam T2 Denominator type.
/// @param n1 Numerator.
/// @param n2 Denominator.
/// @return Result of division using dual arithmetic.
template <class T1, class T2>
  requires number_operands<T1, T2>
inline constexpr auto operator/(const T1 &n1, const T2 &n2) {
  return divides{}(n1, n2);
}

/// @brief `/` element by element: two arrays of the same length, or an array and one value
/// applied to every element. See `elementwise`.
template <class T1, class T2>
  requires elementwise_operands<T1, T2>
inline constexpr auto operator/(const T1 &c1, const T2 &c2) {
  return elementwise<divides>{}(c1, c2);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_DIVIDES_HPP
