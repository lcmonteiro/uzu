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
  template <class T>
  constexpr auto value(const T &v1, const T &v2) const {
    return v1 + v2;
  }

  /// @brief Computes the derivative for addition of two dual numbers.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const duo<T> &n2) const {
    return n1.d + n2.d;
  }

  /// @brief Computes the derivative for addition where only the second operand
  /// is dual.
  template <class T>
  constexpr auto dvalue(const T &, const duo<T> &n2) const {
    return n2.d;
  }

  /// @brief Computes the derivative for addition where only the first operand
  /// is dual.
  template <class T>
  constexpr auto dvalue(const duo<T> &n1, const T &) const {
    return n1.d;
  }
};

/// @brief Overloads the addition operator for dual numbers and scalars.
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
