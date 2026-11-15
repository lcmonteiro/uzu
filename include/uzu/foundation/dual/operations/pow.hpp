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
struct pow_operation : binary_operation<pow_operation> {
  /// @brief Computes the exponentiation of two values.
  /// @tparam T Numeric type.
  /// @param v1 Base value.
  /// @param v2 Exponent value.
  /// @return The result of pow(v1, v2).
  template <class T>
  auto value(const T &v1, const T &v2) const {
    return std::pow(v1, v2);
  }

  /// @brief Derivative of pow(n1.v, n2.v) when base and exponent both carry the index.
  template <class T>
  auto dvalue(const duo<T> &n1, const duo<T> &n2) const {
    return base_term(n1.v, n2.v, n1.d) + exponent_term(n1.v, n2.v, n2.d);
  }

  /// @brief Derivative of pow(v1, n2.v) when only the exponent carries the index.
  template <class T>
  auto dvalue(const T &v1, const duo<T> &n2) const {
    return exponent_term(v1, n2.v, n2.d);
  }

  /// @brief Derivative of pow(n1.v, v2) when only the base carries the index.
  template <class T>
  auto dvalue(const duo<T> &n1, const T &v2) const {
    return base_term(n1.v, v2, n1.d);
  }

 private:
  /// @brief What the base contributes: `y * x^(y-1) * dx`.
  ///
  /// Zero when the exponent is zero: `x^0` is the constant one, and evaluating the formula at
  /// `x = 0` would multiply zero by `0^-1`, which is NaN.
  template <class T>
  static auto base_term(const T &x, const T &y, const T &dx) -> T {
    if (y == T{0}) {
      return T{0};
    }
    return y * std::pow(x, y - T{1}) * dx;
  }

  /// @brief What the exponent contributes: `x^y * ln(x) * dy`.
  ///
  /// Zero when the exponent does not vary (`dy == 0`), whatever the base, and zero at `x = 0`
  /// with a positive exponent, which is the limit of `x^y * ln(x)`. Evaluating the formula in
  /// either case would multiply zero by `ln(0)`, which is NaN. A negative base with a varying
  /// exponent has no real derivative and stays NaN.
  template <class T>
  static auto exponent_term(const T &x, const T &y, const T &dy) -> T {
    if (dy == T{0} || (x == T{0} && y > T{0})) {
      return T{0};
    }
    return std::pow(x, y) * std::log(x) * dy;
  }
};
/// @brief `pow` for dual numbers and scalars. Found by ADL: call it unqualified.
/// @tparam T1 Base type.
/// @tparam T2 Exponent type.
/// @param n1 Base value.
/// @param n2 Exponent value.
/// @return Result of exponentiation using dual arithmetic.
template <class T1, class T2>
  requires number_operands<T1, T2>
inline auto pow(const T1 &n1, const T2 &n2) {
  return std::invoke(pow_operation{}, n1, n2);
}
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
    return pow(n1, n2);
  }
};
/// @brief `pow` for element-wise array exponentiation. Found by ADL: call it unqualified.
/// @tparam T1 Array or element type.
/// @tparam T2 Array or element type.
/// @param n1 Array or element (base).
/// @param n2 Array or element (exponent).
/// @return Array with elements as pow(n1[i], n2[i]).
template <class T1, class T2>
  requires array_operands<T1, T2>
inline auto pow(const T1 &n1, const T2 &n2) {
  return std::invoke(pow_transform{}, n1, n2);
}
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
    return pow(n1, n2);
  }
};
/// @brief `pow` broadcast between an array and a scalar. Found by ADL: call it unqualified.
/// @tparam T1 Array or scalar type (base).
/// @tparam T2 Array or scalar type (exponent).
/// @param c1 Array or scalar (base).
/// @param c2 Array or scalar (exponent).
/// @return Array with elements as pow(c1[i], c2) or pow(c1, c2[i]).
template <class T1, class T2>
  requires broadcast_operands<T1, T2>
inline auto pow(const T1 &c1, const T2 &c2) {
  return std::invoke(pow_broadcast{}, c1, c2);
}
}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_POW_HPP
