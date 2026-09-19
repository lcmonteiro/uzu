/// @file divides.hpp
/// @brief Defines division operations for dual numbers, including derivative
/// computations,
///        element-wise array division, and broadcasting.

#pragma once

#include <functional>
#include "dual/helpers/operations.hpp"

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
  return std::invoke(divides{}, n1, n2);
}

/// @brief Functor for element-wise division transformation on arrays.
struct divides_transform : transform_binary_operation<divides_transform> {
  /// @brief Applies division to each corresponding pair of elements.
  /// @tparam T1 Numerator element type.
  /// @tparam T2 Denominator element type.
  /// @param n1 Numerator element.
  /// @param n2 Denominator element.
  /// @return Result of n1 / n2.
  template <class T1, class T2>
  constexpr auto transform(const T1 &n1, const T2 &n2) const {
    return n1 / n2;
  }
};

/// @brief Overloads the division operator for element-wise array division.
/// @tparam T1 Array or element type.
/// @tparam T2 Array or element type.
/// @param c1 Numerator array or element.
/// @param c2 Denominator array or element.
/// @return Array with elements as c1[i] / c2[i].
template <class T1, class T2>
  requires array_operands<T1, T2>
inline constexpr auto operator/(const T1 &c1, const T2 &c2) {
  return std::invoke(divides_transform{}, c1, c2);
}

/// @brief Functor for broadcasting division operation between arrays and
/// scalars.
struct divides_broadcast : broadcast_operation<divides_broadcast> {
  /// @brief Applies division to each array element and scalar.
  /// @tparam T1 Array element type.
  /// @tparam T2 Scalar type.
  /// @param n1 Array element.
  /// @param n2 Scalar.
  /// @return Result of n1 / n2.
  template <class T1, class T2>
  constexpr auto transform(const T1 &n1, const T2 &n2) const {
    return n1 / n2;
  }
};

/// @brief Overloads the division operator for broadcasting division between
/// arrays and scalars.
/// @tparam T1 Array or scalar type.
/// @tparam T2 Array or scalar type.
/// @param c1 Array or scalar.
/// @param c2 Array or scalar.
/// @return Array with elements as c1[i] / c2 (or c1 / c2[i]).
template <class T1, class T2>
  requires broadcast_operands<T1, T2>
inline constexpr auto operator/(const T1 &c1, const T2 &c2) {
  return std::invoke(divides_broadcast{}, c1, c2);
}

}  // namespace dual
