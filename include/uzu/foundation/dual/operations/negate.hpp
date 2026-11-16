/// ===============================================================================================
/// @file
///
/// @brief Defines negation operations for dual numbers, including derivative
/// computations and element-wise array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_NEGATE_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_NEGATE_HPP

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing the negation and its derivative for dual
/// numbers.
struct negate : unary_operation<negate> {
  /// @brief Computes the negation of a value.
  /// @tparam T The numeric type of the value.
  /// @param v The input value.
  /// @return The negation of v.
  template <class T>
  constexpr auto value(const T &v) const {
    return -v;
  }

  /// @brief Computes the derivative of the negation operation.
  /// @tparam T The numeric type of the value.
  /// @param n A duo containing the input value and its derivative.
  /// @return The negation of the derivative component.
  template <class T>
  constexpr auto dvalue(const duo<T> &n) const {
    return -n.d;
  }
};

/// @brief Overloads the unary minus operator for dual numbers.
/// @tparam T The numeric type.
/// @param n The dual number.
/// @return The result of negating n using dual arithmetic.
template <number_operand T>
inline constexpr auto operator-(const T &n) {
  return negate{}(n);
}

/// @brief Unary `-` of each element of an array. See `elementwise`.
template <array_operand T>
inline constexpr auto operator-(const T &c) {
  return elementwise<negate>{}(c);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_NEGATE_HPP
