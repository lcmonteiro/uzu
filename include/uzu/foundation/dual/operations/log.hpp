/// ===============================================================================================
/// @file
///
/// @brief Defines logarithm operations for dual numbers, including derivative
/// computations and element-wise array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_LOG_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_LOG_HPP

#include <cmath>
#include <functional>
#include <limits>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing the natural logarithm and its derivative for
/// dual numbers.
struct log_operation : unary_operation<log_operation> {
  /// @brief Computes the natural logarithm of a value.
  /// @tparam T The numeric type of the value.
  /// @param v The input value.
  /// @return The natural logarithm of v.
  template <class T>
  auto value(const T &v) const {
    return std::log(v);
  }

  /// @brief Computes the derivative of the logarithm function.
  /// @tparam T The numeric type of the value.
  /// @param n A duo containing the input value and its derivative.
  /// @return The derivative of log(n.v), which is n.d / n.v.
  ///         If n.v <= 0, returns NaN.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    // If the input is invalid for logarithm (n.v <= 0),
    // the derivative should also be NaN
    if (n.v < static_cast<T>(0)) {
      return std::numeric_limits<T>::quiet_NaN();
    }
    return n.d / n.v;  // This handles n.v == 0 case naturally (produces inf)
  }
};
/// @brief `log` for dual numbers. Found by ADL: call it unqualified.
/// @tparam T The numeric type.
/// @param n The dual number.
/// @return The natural logarithm of the dual number.
template <number_operand T>
inline auto log(const T &n) {
  return std::invoke(log_operation{}, n);
}
/// @brief Functor for element-wise logarithm transformation on dual arrays.
struct log_transform : transform_unary_operation<log_transform> {
  /// @brief Applies the logarithm transformation to an array element.
  /// @tparam T The numeric type of the element.
  /// @param n The element.
  /// @return The natural logarithm of the element.
  template <class T>
  auto transform(const T &n) const {
    return log(n);
  }
};
/// @brief `log` for arrays of dual numbers. Found by ADL: call it unqualified.
/// @tparam T The array type.
/// @param n The array of dual numbers.
/// @return An array of logarithms of the dual numbers.
template <array_operand T>
inline auto log(const T &n) {
  return std::invoke(log_transform{}, n);
}
}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_LOG_HPP
