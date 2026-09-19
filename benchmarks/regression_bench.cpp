/// @file bench_regression.cpp
/// @brief A dense problem, for contrast with the bezier fit.
///
///   g++ -std=c++17 -O2 -I.. -DCOEFFS=16 bench_regression.cpp -o bench
///
/// Least squares fit of a polynomial with COEFFS coefficients to SAMPLES
/// points, by gradient descent. Every residual depends on every coefficient,
/// so every intermediate value carries the whole derivative set - the case
/// where dense storage is not mostly zeros.
///
/// Contrast with the bezier fit, which is sparse: there each control point
/// touches its own index and nothing else.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <utility>

#include "uzu/foundation/dual.hpp"

#ifndef COEFFS
#define COEFFS 8
#endif
#ifndef SAMPLES
#define SAMPLES 64
#endif
#ifndef STEPS
#define STEPS 200
#endif
#ifndef RATE
#define RATE (0.1 / SAMPLES)
#endif

#ifndef REPEATS
#define REPEATS 7
#endif

namespace {

using scalar = double;

/// @brief x^0 .. x^(COEFFS-1) for each sample, precomputed.
constexpr auto basis() {
  std::array<std::array<scalar, COEFFS>, SAMPLES> out{};
  for (std::size_t i = 0; i < SAMPLES; ++i) {
    const scalar x = -1.0 + 2.0 * scalar(i) / scalar(SAMPLES - 1);
    scalar p = 1.0;
    for (std::size_t j = 0; j < COEFFS; ++j) {
      out[i][j] = p;
      p *= x;
    }
  }
  return out;
}

/// @brief The curve being fitted.
constexpr auto targets() {
  std::array<scalar, SAMPLES> out{};
  for (std::size_t i = 0; i < SAMPLES; ++i) {
    const scalar x = -1.0 + 2.0 * scalar(i) / scalar(SAMPLES - 1);
    out[i] = 1.0 / (1.0 + 25.0 * x * x);
  }
  return out;
}

/// @brief `sum_j w_j * phi_j`. Dense: the result carries every index.
template <class W, std::size_t... J>
constexpr auto predict(
    const W& w, const std::array<scalar, COEFFS>& phi, std::index_sequence<J...>) {
  using std::get;
  return ((get<J>(w) * phi[J]) + ...);
}

template <class E, std::size_t... J>
constexpr auto gradient_of(const E& e, std::index_sequence<J...>) {
  return std::array<scalar, COEFFS>{e.template dvalue<J>()...};
}

template <class W, std::size_t... J>
constexpr void descend(
    W& w, const std::array<scalar, COEFFS>& g, scalar rate, std::index_sequence<J...>) {
  using std::get;
  ((get<J>(w).value(get<J>(w).value() - rate * g[J])), ...);
}

}  // namespace

int main() {
  constexpr auto phi = basis();
  constexpr auto ys = targets();
  constexpr auto seq = std::make_index_sequence<COEFFS>{};
  const scalar rate = RATE;

  auto best = std::chrono::duration<double>::max();
  scalar sink = 0.0;
  for (int r = 0; r < REPEATS; ++r) {
    auto w = dual::make_array<0>(std::array<scalar, COEFFS>{});
    const auto t0 = std::chrono::steady_clock::now();
    for (int step = 0; step < STEPS; ++step) {
      auto total = [&] {
        auto acc = (ys[0] - predict(w, phi[0], seq)) * (ys[0] - predict(w, phi[0], seq));
        for (std::size_t i = 1; i < SAMPLES; ++i) {
          const auto d = ys[i] - predict(w, phi[i], seq);
          acc = acc + d * d;
        }
        return acc;
      }();
      descend(w, gradient_of(total, seq), rate, seq);
    }
    const auto t1 = std::chrono::steady_clock::now();
    best = std::min(best, std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0));
    sink += w.to_array()[0] + w.to_array()[COEFFS / 2];
  }

  std::printf(
      "coeffs=%d samples=%d steps=%d best=%.3f ms  check=%.6f\n",
      COEFFS,
      SAMPLES,
      STEPS,
      best.count() * 1e3,
      sink / REPEATS);
  return 0;
}
