/// ===============================================================================================
/// @file
///
/// @brief Defines square root operations for dual numbers, including derivative
/// computations and element-wise array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_SQRT_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_SQRT_HPP

#include <cmath>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing square root and its derivative for dual
/// numbers.
struct sqrt_operation : unary_operation<sqrt_operation> {
  /// @brief Computes the square root of a value.
  template <class T>
  auto value(const T &v) const {
    return std::sqrt(v);
  }

  /// @brief Computes the derivative of the square root operation.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return n.d / (static_cast<T>(2.0) * std::sqrt(n.v));
  }
};

/// @brief `sqrt` for dual numbers. Found by ADL: call it unqualified.
template <number_operand T>
inline auto sqrt(const T &n) {
  return sqrt_operation{}(n);
}

/// @brief `sqrt` of each element of an array. See `elementwise`. Found by ADL: call it unqualified.
template <array_operand T>
inline auto sqrt(const T &c) {
  return elementwise<sqrt_operation>{}(c);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_SQRT_HPP
