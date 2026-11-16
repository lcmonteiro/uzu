/// ===============================================================================================
/// @file
///
/// @brief Defines addition operations for dual numbers, including derivative
/// computations, element-wise array addition, and broadcasting.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_PLUS_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_PLUS_HPP

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Binary functor for addition and its derivatives for dual numbers.
struct plus : binary_operation<plus> {
  /// @brief Computes the sum of two values.
  /// @tparam T The numeric type.
  /// @param v1 First value (left operand).
  /// @param v2 Second value (right operand).
  /// @return The result of v1 + v2.
  template <class T>
  constexpr auto value(const T &v1, const T &v2) const {
    return v1 + v2;
  }

  /// @brief Computes the derivative for addition of two dual numbers.
  /// @tparam T Numeric type.
  /// @param n1 First dual value and derivative.
  /// @param n2 Second dual value and derivative.
  /// @return The derivative of n1.v + n2.v.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const duo<T> &n2) const {
    return n1.d + n2.d;
  }

  /// @brief Computes the derivative for addition where only the second operand
  /// is dual.
  /// @tparam T Numeric type.
  /// @param v1 First value (scalar).
  /// @param n2 Second dual value and derivative.
  /// @return The derivative of v1 + n2.v.
  template <class T>
  constexpr auto dvalue(const T &, const duo<T> &n2) const {
    return n2.d;
  }

  /// @brief Computes the derivative for addition where only the first operand
  /// is dual.
  /// @tparam T Numeric type.
  /// @param n1 First dual value and derivative.
  /// @param v2 Second value (scalar).
  /// @return The derivative of n1.v + v2.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const T &) const {
    return n1.d;
  }
};

/// @brief Overloads the addition operator for dual numbers and scalars.
/// @tparam T1 First operand type.
/// @tparam T2 Second operand type.
/// @param n1 First operand.
/// @param n2 Second operand.
/// @return Result of addition using dual arithmetic.
template <class T1, class T2>
  requires number_operands<T1, T2>
inline constexpr auto operator+(const T1 &n1, const T2 &n2) {
  return plus{}(n1, n2);
}

/// @brief `+` element by element: two arrays of the same length, or an array and one value
/// applied to every element. See `elementwise`.
template <class T1, class T2>
  requires elementwise_operands<T1, T2>
inline constexpr auto operator+(const T1 &c1, const T2 &c2) {
  return elementwise<plus>{}(c1, c2);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_PLUS_HPP
