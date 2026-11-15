/// ===============================================================================================
/// @file
///
/// @brief Defines cosine operations for dual numbers, including derivative
/// computations and array transforms.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_COS_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_COS_HPP

#include <cmath>
#include <functional>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Functor for computing the cosine and its derivative for dual numbers.
struct cos_operation : unary_operation<cos_operation> {
  /// @brief Computes the cosine of a value.
  /// @tparam T The numeric type of the value.
  /// @param v The input value.
  /// @return The cosine of v.
  template <class T>
  auto value(const T &v) const {
    return std::cos(v);
  }

  /// @brief Computes the derivative of the cosine function.
  /// @tparam T The numeric type of the value.
  /// @param n A duo containing the input value and its derivative.
  /// @return The derivative of cos(n.v), which is -sin(n.v) * n.d.
  template <class T>
  auto dvalue(const duo<T> &n) const {
    return -std::sin(n.v) * n.d;
  }
};
/// @brief `cos` for dual numbers. Found by ADL: call it unqualified.
/// @tparam T The numeric type.
/// @param n The dual number.
/// @return The cosine of the dual number.
template <number_operand T>
inline auto cos(const T &n) {
  return std::invoke(cos_operation{}, n);
}
/// @brief Functor for element-wise cosine transformation on dual arrays.
struct cos_transform : transform_unary_operation<cos_transform> {
  /// @brief Applies the cosine transformation to an array element.
  /// @tparam T The numeric type of the element.
  /// @param n The element.
  /// @return The cosine of the element.
  template <class T>
  auto transform(const T &n) const {
    return cos(n);
  }
};
/// @brief `cos` for arrays of dual numbers. Found by ADL: call it unqualified.
/// @tparam T The array type.
/// @param n The array of dual numbers.
/// @return An array of cosines of the dual numbers.
template <array_operand T>
inline auto cos(const T &n) {
  return std::invoke(cos_transform{}, n);
}
}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_COS_HPP
