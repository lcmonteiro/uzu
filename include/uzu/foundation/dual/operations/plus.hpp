/// ===============================================================================================
/// @file
///
/// @brief Defines addition operations for dual numbers, including derivative
/// computations, element-wise array addition, and broadcasting.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_PLUS_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_PLUS_HPP

#include <functional>

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
  return std::invoke(plus{}, n1, n2);
}

/// @brief Functor for element-wise addition transformation on arrays.
struct plus_transform : transform_binary_operation<plus_transform> {
  /// @brief Applies addition to each corresponding pair of elements.
  /// @tparam T1 First element type.
  /// @tparam T2 Second element type.
  /// @param n1 First element.
  /// @param n2 Second element.
  /// @return Result of n1 + n2.
  template <class T1, class T2>
  constexpr auto transform(const T1 &n1, const T2 &n2) const {
    return n1 + n2;
  }
};

/// @brief Overloads the addition operator for element-wise array addition.
/// @tparam T1 Array or element type.
/// @tparam T2 Array or element type.
/// @param c1 First array or element.
/// @param c2 Second array or element.
/// @return Array with elements as c1[i] + c2[i].
template <class T1, class T2>
  requires array_operands<T1, T2>
inline constexpr auto operator+(const T1 &c1, const T2 &c2) {
  return std::invoke(plus_transform{}, c1, c2);
}

/// @brief Functor for broadcasting addition operation between arrays and
/// scalars.
struct plus_broadcast : broadcast_operation<plus_broadcast> {
  /// @brief Applies addition to each array element and scalar.
  /// @tparam T1 Array element type.
  /// @tparam T2 Scalar type.
  /// @param n1 Array element.
  /// @param n2 Scalar.
  /// @return Result of n1 + n2.
  template <class T1, class T2>
  constexpr auto transform(const T1 &n1, const T2 &n2) const {
    return n1 + n2;
  }
};

/// @brief Overloads the addition operator for broadcasting addition between
/// arrays and scalars.
/// @tparam T1 Array or scalar type.
/// @tparam T2 Array or scalar type.
/// @param c1 Array or scalar.
/// @param c2 Array or scalar.
/// @return Array with elements as c1[i] + c2 (or c1 + c2[i]).
template <class T1, class T2>
  requires broadcast_operands<T1, T2>
inline constexpr auto operator+(const T1 &c1, const T2 &c2) {
  return std::invoke(plus_broadcast{}, c1, c2);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_PLUS_HPP
