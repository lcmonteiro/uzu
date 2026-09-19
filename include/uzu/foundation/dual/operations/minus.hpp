/// ===============================================================================================
/// @file
///
/// @brief Defines subtraction operations for dual numbers, including derivative
/// computations, element-wise array subtraction, and broadcasting.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_MINUS_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_MINUS_HPP

#include <functional>

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
  return std::invoke(minus{}, n1, n2);
}

/// @brief Functor for element-wise subtraction transformation on arrays.
struct minus_transform : transform_binary_operation<minus_transform> {
  /// @brief Applies subtraction to each corresponding pair of elements.
  /// @tparam T1 Minuend element type.
  /// @tparam T2 Subtrahend element type.
  /// @param n1 Minuend element.
  /// @param n2 Subtrahend element.
  /// @return Result of n1 - n2.
  template <class T1, class T2>
  constexpr auto transform(const T1 &n1, const T2 &n2) const {
    return n1 - n2;
  }
};

/// @brief Overloads the subtraction operator for element-wise array
/// subtraction.
/// @tparam T1 Array or element type.
/// @tparam T2 Array or element type.
/// @param c1 Minuend array or element.
/// @param c2 Subtrahend array or element.
/// @return Array with elements as c1[i] - c2[i].
template <class T1, class T2>
  requires array_operands<T1, T2>
inline constexpr auto operator-(const T1 &c1, const T2 &c2) {
  return std::invoke(minus_transform{}, c1, c2);
}

/// @brief Functor for broadcasting subtraction operation between arrays and
/// scalars.
struct minus_broadcast : broadcast_operation<minus_broadcast> {
  /// @brief Applies subtraction to each array element and scalar.
  /// @tparam T1 Array element type.
  /// @tparam T2 Scalar type.
  /// @param n1 Array element.
  /// @param n2 Scalar.
  /// @return Result of n1 - n2.
  template <class T1, class T2>
  constexpr auto transform(const T1 &n1, const T2 &n2) const {
    return n1 - n2;
  }
};

/// @brief Overloads the subtraction operator for broadcasting subtraction
/// between arrays and scalars.
/// @tparam T1 Array or scalar type.
/// @tparam T2 Array or scalar type.
/// @param c1 Array or scalar.
/// @param c2 Array or scalar.
/// @return Array with elements as c1[i] - c2 (or c1 - c2[i]).
template <class T1, class T2>
  requires broadcast_operands<T1, T2>
inline constexpr auto operator-(const T1 &c1, const T2 &c2) {
  return std::invoke(minus_broadcast{}, c1, c2);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_MINUS_HPP
