/// ===============================================================================================
/// @file
///
/// @brief Defines sine operations for dual numbers, including derivative
/// computations and element-wise array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_SIN_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_SIN_HPP

#include <cmath>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing sine and its derivative for dual numbers.
struct sin_operation : unary_operation<sin_operation> {
  /// @brief Computes the sine of a value.
  template <class T>
  auto value(const T &v) const {
    return std::sin(v);
  }

  /// @brief Computes the derivative of the sine operation.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return std::cos(n.v) * n.d;
  }
};

/// @brief `sin` for dual numbers. Found by ADL: call it unqualified.
template <number_operand T>
inline auto sin(const T &n) {
  return sin_operation{}(n);
}

/// @brief `sin` of each element of an array. See `elementwise`. Found by ADL: call it unqualified.
template <array_operand T>
inline auto sin(const T &c) {
  return elementwise<sin_operation>{}(c);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_SIN_HPP
