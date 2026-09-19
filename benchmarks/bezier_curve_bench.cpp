/// @file bench_bezier_curve.cpp
/// @brief Compile time probe for bezier_curve::fit, parametrised by the
/// number of points being fitted.
///
/// fit() seeds one dual index per control point (6 for x, 6 for y) plus one
/// per fitted point, so the derivative set has 12 + POINTS indices and the
/// error expression merges all of them. Sweeping POINTS therefore sweeps the
/// width of the index-set metaprogramming.
///
///   g++ -std=c++17 -O2 -I.. -DPOINTS=12 -c bench_bezier_curve.cpp

#include <cstdio>

#include "uzu/foundation/types/bezier_curve.hpp"

#ifndef POINTS
#define POINTS 8
#endif

static_assert(POINTS >= 2, "range<T, N>() divides by N - 1");

namespace {

template <std::size_t... Is>
constexpr auto make_xs(std::index_sequence<Is...>) {
  return std::array{(1.0 + static_cast<double>(Is % 3))...};
}

template <std::size_t... Is>
constexpr auto make_ys(std::index_sequence<Is...>) {
  return std::array{(1.0 + static_cast<double>(Is) * 0.5)...};
}

}  // namespace

int main() {
  auto curve = dual::bezier_curve<double>{};
  const auto xs = make_xs(std::make_index_sequence<POINTS>{});
  const auto ys = make_ys(std::make_index_sequence<POINTS>{});

  curve.fit(xs, ys, 20, 0.5);

  std::printf(
      "points=%d dim=%d x(0.5)=%g y(0.5)=%g\n",
      POINTS,
      POINTS + 12,
      curve.get_x(0.5),
      curve.get_y(0.5));
  return 0;
}
