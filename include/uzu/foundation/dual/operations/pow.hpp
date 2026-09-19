/// ===============================================================================================
/// @file
///
/// @brief Defines exponentiation operations for dual numbers, including
/// derivative computations, element-wise array exponentiation, and broadcasting.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_POW_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_POW_HPP

#include <cmath>
#include <functional>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Binary functor for exponentiation and its derivatives for dual
/// numbers.
struct pow : binary_operation<pow> {
  /// @brief Computes the exponentiation of two values.
  /// @tparam T Numeric type.
  /// @param v1 Base value.
  /// @param v2 Exponent value.
  /// @return The result of pow(v1, v2).
  template <class T>
  auto value(const T &v1, const T &v2) const {
    return std::pow(v1, v2);
  }

  /// @brief Computes the derivative for exponentiation of two dual numbers.
  /// @tparam T Numeric type.
  /// @param n1 Dual base value and derivative.
  /// @param n2 Dual exponent value and derivative.
  /// @return Derivative of pow(n1.v, n2.v).
  template <class T>
  auto dvalue(const duo<T> &n1, const duo<T> &n2) const {
    return std::pow(n1.v, n2.v - 1) * (n2.v * n1.d + n1.v * n2.d * std::log(n1.v));
  }

  /// @brief Computes the derivative for exponentiation where only the exponent
  /// is dual.
  /// @tparam T Numeric type.
  /// @param v1 Base value.
  /// @param n2 Dual exponent value and derivative.
  /// @return Derivative of pow(v1, n2.v).
  template <class T>
  auto dvalue(const T &v1, const duo<T> &n2) const {
    return n2.d * std::pow(v1, n2.v) * std::log(v1);
  }

  /// @brief Computes the derivative for exponentiation where only the base is
  /// dual.
  /// @tparam T Numeric type.
  /// @param n1 Dual base value and derivative.
  /// @param v2 Exponent value.
  /// @return Derivative of pow(n1.v, v2).
  template <class T>
  auto dvalue(const duo<T> &n1, const T &v2) const {
    return v2 * std::pow(n1.v, v2 - 1) * n1.d;
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the pow function for dual numbers and scalars.
/// @tparam T1 Base type.
/// @tparam T2 Exponent type.
/// @param n1 Base value.
/// @param n2 Exponent value.
/// @return Result of exponentiation using dual arithmetic.
template <class T1, class T2>
  requires dual::number_operands<T1, T2>
inline auto pow(const T1 &n1, const T2 &n2) {
  return std::invoke(dual::pow{}, n1, n2);
}
}  // namespace std

namespace dual {
/// @brief Functor for element-wise exponentiation transformation on arrays.
struct pow_transform : transform_binary_operation<pow_transform> {
  /// @brief Applies exponentiation to each corresponding pair of elements.
  /// @tparam T1 Base element type.
  /// @tparam T2 Exponent element type.
  /// @param n1 Base element.
  /// @param n2 Exponent element.
  /// @return Result of pow(n1, n2).
  template <class T1, class T2>
  auto transform(const T1 &n1, const T2 &n2) const {
    return std::pow(n1, n2);
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the pow function for element-wise array exponentiation.
/// @tparam T1 Array or element type.
/// @tparam T2 Array or element type.
/// @param n1 Array or element (base).
/// @param n2 Array or element (exponent).
/// @return Array with elements as pow(n1[i], n2[i]).
template <class T1, class T2>
  requires dual::array_operands<T1, T2>
inline auto pow(const T1 &n1, const T2 &n2) {
  return std::invoke(dual::pow_transform{}, n1, n2);
}
}  // namespace std

namespace dual {
/// @brief Functor for broadcasting exponentiation operation between arrays and
/// scalars.
struct pow_broadcast : broadcast_operation<pow_broadcast> {
  /// @brief Applies exponentiation to each array element and scalar.
  /// @tparam T1 Array element type.
  /// @tparam T2 Scalar type.
  /// @param n1 Array element (base).
  /// @param n2 Scalar (exponent).
  /// @return Result of pow(n1, n2).
  template <class T1, class T2>
  auto transform(const T1 &n1, const T2 &n2) const {
    return std::pow(n1, n2);
  }
};
}  // namespace dual

namespace std {
/// @brief Overloads the pow function for broadcasting exponentiation between
/// arrays and scalars.
/// @tparam T1 Array or scalar type (base).
/// @tparam T2 Array or scalar type (exponent).
/// @param c1 Array or scalar (base).
/// @param c2 Array or scalar (exponent).
/// @return Array with elements as pow(c1[i], c2) or pow(c1, c2[i]).
template <class T1, class T2>
  requires dual::broadcast_operands<T1, T2>
inline auto pow(const T1 &c1, const T2 &c2) {
  return std::invoke(dual::pow_broadcast{}, c1, c2);
}
}  // namespace std

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_POW_HPP
