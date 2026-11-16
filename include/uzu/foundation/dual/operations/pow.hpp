/// ===============================================================================================
/// @file
///
/// @brief Defines exponentiation operations for dual numbers, including
/// derivative computations, element-wise array exponentiation, and broadcasting.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_POW_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_POW_HPP

#include <cmath>

#include "uzu/foundation/dual/operations/base.hpp"

namespace dual {
/// @brief Binary functor for exponentiation and its derivatives for dual
/// numbers.
struct pow_operation : binary_operation<pow_operation> {
  /// @brief Computes the exponentiation of two values.
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
template <class T1, class T2>
  requires number_operands<T1, T2>
inline auto pow(const T1 &n1, const T2 &n2) {
  return pow_operation{}(n1, n2);
}

/// @brief `pow` element by element: two arrays of the same length, or an array and one value
/// applied to every element. See `elementwise`. Found by ADL: call it unqualified.
template <class T1, class T2>
  requires elementwise_operands<T1, T2>
inline auto pow(const T1 &c1, const T2 &c2) {
  return elementwise<pow_operation>{}(c1, c2);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_POW_HPP
