/// ===============================================================================================
/// @file
///
/// @brief A 5th-order Bezier curve fitted by gradient descent, used as the wide-index
/// benchmark: `fit` seeds one dual index per control point plus one per fitted point, so the
/// derivative set is `12 + POINTS` wide and the error expression merges all of it.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_TYPES_BEZIER_CURVE_HPP
#define UZU_FOUNDATION_TYPES_BEZIER_CURVE_HPP

#include "uzu/foundation/dual/functional/concat.hpp"
#include "uzu/foundation/dual/functional/summation.hpp"
#include "uzu/foundation/dual/functional/zip.hpp"
#include "uzu/foundation/dual/operations/divides.hpp"
#include "uzu/foundation/dual/operations/exp.hpp"
#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"
#include "uzu/foundation/dual/operations/pow.hpp"
#include "uzu/foundation/dual/print.hpp"

namespace dual {
namespace detail {
template <class T, size_t... Is>
constexpr auto iota(std::index_sequence<Is...>) {
  return std::array{T{Is}...};
}
template <class T, size_t N, size_t... Is>
constexpr auto range(std::index_sequence<Is...>) {
  return std::array{(T{Is} / N)...};
}
template <class T, size_t... Dn, size_t... Is>
constexpr auto dratio(const number<T, Dn...> &n, std::index_sequence<Is...>) {
  constexpr auto lr = sizeof...(Dn);
  return std::array{T{n.value() / (n.template dvalue<Is>() * lr * lr)}...};
}

template <class T, size_t... Dn, size_t... Is>
constexpr auto dvalue(const number<T, Dn...> &n, T lr, std::index_sequence<Is...>) {
  return std::array{(lr * n.template dvalue<Is>())...};
}

}  // namespace detail

template <class T, size_t N>
constexpr auto iota() {
  return detail::iota<T>(std::make_index_sequence<N>{});
}

template <class T, size_t N>
constexpr auto range() {
  return detail::range<T, N - 1>(std::make_index_sequence<N>{});
}

template <size_t N, class T, size_t... Dn>
constexpr auto dratio(const number<T, Dn...> &n) {
  return detail::dratio(n, std::make_index_sequence<N>{});
}

template <size_t N, class T, size_t... Dn>
constexpr auto dvalue(const number<T, Dn...> &n, T lr) {
  return detail::dvalue(n, lr, std::make_index_sequence<N>{});
}

template <class T>
constexpr auto sigmoid(const T &n) {
  return 1.0 / (1.0 + std::exp(-1.0 * n));
}

// 5th-order Bézier curve
namespace detail {
template <class Number, class Vector>
constexpr auto curve_line_value(const Number &t, const Vector &c) {
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
constexpr auto curve_line_first_derivative(const Number &t, const Vector &c) {
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
constexpr auto curve_line_second_derivative(const Number &t, const Vector &c) {
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
constexpr auto curve_line_closest(
    const Number &x, const Number &y, const VectorX &cx, const VectorY &cy,
    const size_t iter = 10) {
  auto t = Number{0.5};
  for (size_t i = 0; i < iter; ++i) {
    const auto p_x = detail::curve_line_value(t, cx);
    const auto p_y = detail::curve_line_value(t, cy);
    const auto v_x = detail::curve_line_first_derivative(t, cx);
    const auto v_y = detail::curve_line_first_derivative(t, cy);
    const auto a_x = detail::curve_line_second_derivative(t, cx);
    const auto a_y = detail::curve_line_second_derivative(t, cy);
    const auto r_x = p_x - x;
    const auto r_y = p_y - y;

    const auto f = r_x * v_x + r_y * v_y;
    const auto df = v_x * v_x + v_y * v_y + r_x * a_x + r_y * a_y;

    if (std::abs(f) < std::numeric_limits<Number>::epsilon()) {
      break;
    }
    if (std::abs(df) < std::numeric_limits<Number>::epsilon()) {
      break;
    }
    t -= f / df;
    t = std::clamp(t, 0.0, 1.0);
  }
  return t;
}
}  // namespace detail

template <class T>
class bezier_curve {
  static constexpr auto DIM = 6;

 public:
  using array = std::array<T, DIM>;

  bezier_curve() = default;
  bezier_curve(const array &cx, const array &cy) : cx_{cx}, cy_{cy} {
  }

  template <class U>
  auto get_x(const U &t) const {
    return detail::curve_line_value(t, cx_);
  }
  template <class U>
  auto get_y(const U &t) const {
    return detail::curve_line_value(t, cy_);
  }

  template <size_t N>
  auto fit(const std::array<T, N> &x, const std::array<T, N> &y, const size_t steps, const T rate);

 private:
  array cx_{iota<T, DIM>()};
  array cy_{iota<T, DIM>()};
};

template <class T>
template <size_t N>
auto bezier_curve<T>::fit(
    const std::array<T, N> &xs, const std::array<T, N> &ys, const size_t steps, const T rate) {
  auto cx = make_array<0>(this->cx_);
  auto cy = make_array<cx.size()>(this->cy_);
  auto ts = make_array<cx.size() + cy.size()>(range<T, N>());

  auto error_function = [&cx, &cy](const auto &xs, const auto &ys, const auto &ts) {
    return dual::summation(zip(xs, ys, ts), [&](const auto &x, const auto &y, const auto &t) {
      const auto st = sigmoid(t);
      const auto dx = x - detail::curve_line_value(st, cx);
      const auto dy = y - detail::curve_line_value(st, cy);
      return dx * dx + dy * dy;
    });
  };

  constexpr auto DIM = cx.size() + cy.size() + ts.size();
  for (size_t i = 0; i < steps; ++i) {
    const auto error = error_function(xs, ys, ts);
    const auto delta = dvalue<DIM>(error, rate);
    concat(cx, cy, ts) = concat(cx, cy, ts) - delta;
  }
  this->cx_ = cx.to_array();
  this->cy_ = cy.to_array();

  return ts.to_array();
}

// template<class Model, class Error]>

}  // namespace dual

#endif  // UZU_FOUNDATION_TYPES_BEZIER_CURVE_HPP
