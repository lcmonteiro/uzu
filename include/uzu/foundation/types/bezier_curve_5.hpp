/// ===============================================================================================
/// @file
///
/// @brief A plain 5th-order Bezier evaluation, free of dual numbers, kept as the scalar
/// reference the fitted curve is read against.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_TYPES_BEZIER_CURVE_5_HPP
#define UZU_FOUNDATION_TYPES_BEZIER_CURVE_5_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <utility>

// 5th-order Bézier curve
namespace bezier {
namespace detail {

/// @brief 0, 1, ... N-1 as an array of T.
///
/// This header referenced `iota<T, DIM>()` without ever declaring it, which no compiler accepted
/// -- GCC rejects the non-dependent name outright and Clang only got away with it because the
/// member initializer was never instantiated. It is spelled the way `bezier_curve.hpp` spells it,
/// which is where this one was extracted from.
template <class T, std::size_t... Is>
constexpr auto iota(std::index_sequence<Is...>) {
  return std::array{T{Is}...};
}

template <class Number, class Vector>
constexpr auto curve_value(const Number& t, const Vector& c) {
  const auto u0 = 1.0 - t;
  const auto u1 = u0 * u0;
  const auto u2 = u1 * u0;
  const auto u3 = u2 * u0;
  const auto u4 = u3 * u0;
  const auto t0 = t;
  const auto t1 = t0 * t0;
  const auto t2 = t1 * t0;
  const auto t3 = t2 * t0;
  const auto t4 = t3 * t0;
  const auto v0 = u4 * std::get<0>(c);
  const auto v1 = +5.0 * u3 * t0 * std::get<1>(c);
  const auto v2 = 10.0 * u2 * t1 * std::get<2>(c);
  const auto v3 = 10.0 * u1 * t2 * std::get<3>(c);
  const auto v4 = +5.0 * u0 * t3 * std::get<4>(c);
  const auto v5 = t4 * std::get<5>(c);
  return v0 + v1 + v2 + v3 + v4 + v5;
}

template <class Number, class Vector>
constexpr auto curve_first_derivative(const Number& t, const Vector& c) {
  const auto u0 = 1.0 - t;
  const auto u1 = u0 * u0;
  const auto u2 = u1 * u0;
  const auto u3 = u2 * u0;
  const auto t0 = t;
  const auto t1 = t0 * t0;
  const auto t2 = t1 * t0;
  const auto t3 = t2 * t0;
  const auto d0 = +5.0 * u3 * (std::get<1>(c) - std::get<0>(c));
  const auto d1 = 20.0 * u2 * t0 * (std::get<2>(c) - std::get<1>(c));
  const auto d2 = 30.0 * u1 * t1 * (std::get<3>(c) - std::get<2>(c));
  const auto d3 = 20.0 * t2 * u0 * (std::get<4>(c) - std::get<3>(c));
  const auto d4 = +5.0 * t3 * (std::get<5>(c) - std::get<4>(c));
  return d0 + d1 + d2 + d3 + d4;
}

template <class Number, class Vector>
constexpr auto curve_second_derivative(const Number& t, const Vector& c) {
  const auto u0 = 1.0 - t;
  const auto u1 = u0 * u0;
  const auto u2 = u1 * u0;
  const auto t0 = t;
  const auto t1 = t0 * t0;
  const auto t2 = t1 * t0;

  const auto d0 = 20.0 * u2 * (std::get<2>(c) - 2.0 * std::get<1>(c) + std::get<0>(c));
  const auto d1 = 60.0 * u1 * t0 * (std::get<3>(c) - 2.0 * std::get<2>(c) + std::get<1>(c));
  const auto d2 = 60.0 * u0 * t1 * (std::get<4>(c) - 2.0 * std::get<3>(c) + std::get<2>(c));
  const auto d3 = 20.0 * t2 * (std::get<5>(c) - 2.0 * std::get<4>(c) + std::get<3>(c));
  return d0 + d1 + d2 + d3;
}

template <class Number, class VectorX, class VectorY>
constexpr auto curve_closest(
    const Number& x, const Number& y, const VectorX& cx, const VectorY& cy,
    const size_t iter = 10) {
  auto t = Number{0.5};
  for (size_t i = 0; i < iter; ++i) {
    const auto p_x = detail::curve_value(t, cx);
    const auto p_y = detail::curve_value(t, cy);
    const auto v_x = detail::curve_first_derivative(t, cx);
    const auto v_y = detail::curve_first_derivative(t, cy);
    const auto a_x = detail::curve_second_derivative(t, cx);
    const auto a_y = detail::curve_second_derivative(t, cy);
    const auto r_x = p_x - x;
    const auto r_y = p_y - y;

    const auto f = r_x * v_x + r_y * v_y;
    if (std::abs(f) < std::numeric_limits<Number>::epsilon()) {
      break;
    }
    const auto df = v_x * v_x + v_y * v_y + r_x * a_x + r_y * a_y;
    if (std::abs(df) < std::numeric_limits<Number>::epsilon()) {
      break;
    }
    t -= f / df;
    t = std::clamp(t, 0.0, 1.0);
  }
  return t;
}
}  // namespace detail

/// @brief 0, 1, ... N-1 as an array of T.
template <class T, std::size_t N>
constexpr auto iota() {
  return detail::iota<T>(std::make_index_sequence<N>{});
}

template <class T>
class curve {
  static constexpr auto DIM = 6;

 public:
  using array = std::array<T, DIM>;

  curve() = default;
  curve(const array& cx, const array& cy) : cx_{cx}, cy_{cy} {
  }

  template <class U>
  auto get_x(const U& t) const {
    return detail::curve_value(t, cx_);
  }
  template <class U>
  auto get_y(const U& t) const {
    return detail::curve_value(t, cy_);
  }

  template <class U>
  auto closest(const U& x, const U& y) {
    return detail::curve_closest(x, y, cx_, cy_);
  }

 private:
  array cx_{iota<T, DIM>()};
  array cy_{iota<T, DIM>()};
};

}  // namespace bezier

#endif  // UZU_FOUNDATION_TYPES_BEZIER_CURVE_5_HPP
