/// ===============================================================================================
/// @file
///
/// @brief Defines cosine operations for dual numbers, including derivative
/// computations and array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_COS_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_COS_HPP

#include <cmath>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing the cosine and its derivative for dual numbers.
struct cos_operation : unary_operation<cos_operation> {
  /// @brief Computes the cosine of a value.
  template <class T>
  auto value(const T &v) const {
    return std::cos(v);
  }

  /// @brief Computes the derivative of the cosine function.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return -std::sin(n.v) * n.d;
  }
};

/// @brief `cos` for dual numbers. Found by ADL: call it unqualified.
template <number_operand T>
inline auto cos(const T &n) {
  return cos_operation{}(n);
}

/// @brief `cos` of each element of an array. See `elementwise`. Found by ADL: call it unqualified.
template <array_operand T>
inline auto cos(const T &c) {
  return elementwise<cos_operation>{}(c);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_COS_HPP
