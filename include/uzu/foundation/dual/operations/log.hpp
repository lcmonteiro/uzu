/// ===============================================================================================
/// @file
///
/// @brief Defines logarithm operations for dual numbers, including derivative
/// computations and element-wise array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_LOG_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_LOG_HPP

#include <cmath>
#include <limits>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing the natural logarithm and its derivative for
/// dual numbers.
struct log_operation : unary_operation<log_operation> {
  /// @brief Computes the natural logarithm of a value.
  template <class T>
  auto value(const T &v) const {
    return std::log(v);
  }

  /// @brief The derivative of the logarithm, `n.d / n.v`.
  ///
  /// NaN for a negative value, where the logarithm has no real derivative; an infinity at zero,
  /// which is the limit there.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    if (n.v < static_cast<T>(0)) {
      return std::numeric_limits<T>::quiet_NaN();
    }
    return n.d / n.v;
  }
};

/// @brief `log` for dual numbers. Found by ADL: call it unqualified.
template <number_operand T>
inline auto log(const T &n) {
  return log_operation{}(n);
}

/// @brief `log` of each element of an array. See `elementwise`. Found by ADL: call it unqualified.
template <array_operand T>
inline auto log(const T &c) {
  return elementwise<log_operation>{}(c);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_LOG_HPP
