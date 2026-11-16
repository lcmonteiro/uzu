/// ===============================================================================================
/// @file
///
/// @brief Defines exponential operations for dual numbers, including derivative
/// computations and element-wise array transforms.
///
/// This is the one operation that cannot run in a constant expression: it bottoms out in
/// `std::exp`, and none of `<cmath>` is `constexpr` before C++26. An expression that reaches for
/// it is a run-time expression, whatever the functions around it are marked.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_EXP_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_EXP_HPP

#include <cmath>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing the exponential function and its derivative for
/// dual numbers.
struct exp_operation : unary_operation<exp_operation> {
  /// @brief Computes the exponential of a value.
  template <class T>
  auto value(const T &v) const {
    return std::exp(v);
  }

  /// @brief Computes the derivative of the exponential function.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return std::exp(n.v) * n.d;
  }
};

/// @brief `exp` for dual numbers. Found by ADL: call it unqualified.
template <number_operand T>
inline auto exp(const T &n) {
  return exp_operation{}(n);
}

/// @brief `exp` of each element of an array. See `elementwise`. Found by ADL: call it unqualified.
template <array_operand T>
inline auto exp(const T &c) {
  return elementwise<exp_operation>{}(c);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_EXP_HPP
