/// @file bench_bezier_runtime.cpp
/// @brief Runtime cost of the bezier fit, for comparison against the other
/// library. Kept separate from the compile time benchmark so that one stays
/// free of <chrono> and its published numbers remain comparable.
///
///   g++ -std=c++17 -O2 -I.. -DPOINTS=16 bench_bezier_runtime.cpp -o bench
///
/// Reports the best of REPEATS runs of STEPS gradient steps, from a fresh
/// curve each time.
///
/// RATE is normalised by the point count. The error is a sum over the points,
/// so its gradient grows with them: a fixed step that converges at 24 points
/// diverges to NaN at 100. Dividing by POINTS keeps the step comparable
/// across the sweep, in both libraries alike.

#include <algorithm>
#include <chrono>
#include <cstdio>

#include "dual/types/bezier_curve.hpp"

#ifndef POINTS
#define POINTS 8
#endif
#ifndef STEPS
#define STEPS 200
#endif
#ifndef RATE
#define RATE (1.0 / POINTS)
#endif
#ifndef REPEATS
#define REPEATS 7
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
  const auto xs = make_xs(std::make_index_sequence<POINTS>{});
  const auto ys = make_ys(std::make_index_sequence<POINTS>{});

  auto best = std::chrono::duration<double>::max();
  double sink = 0.0;
  for (int r = 0; r < REPEATS; ++r) {
    auto curve = dual::bezier_curve<double>{};
    const auto t0 = std::chrono::steady_clock::now();
    curve.fit(xs, ys, STEPS, RATE);
    const auto t1 = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
    best = std::min(best, elapsed);
    sink += curve.get_x(0.5) + curve.get_y(0.5);
  }

  std::printf("points=%d dim=%d steps=%d best=%.3f ms  %.1f us/step  check=%.6f\n",
              POINTS, POINTS + 12, STEPS, best.count() * 1e3,
              best.count() * 1e6 / STEPS, sink / REPEATS);
  return 0;
}
