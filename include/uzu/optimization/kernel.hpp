/// ===============================================================================================
/// @file
///
/// @brief The radial kernel, used both to robustify an edge's residual and to normalise a node's
/// gradient.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_KERNEL_HPP
#define UZU_OPTIMIZATION_KERNEL_HPP

#include <array>
#include <cstddef>

#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"

namespace uzu {
namespace kernel {

/// @brief How many squarings the kernel is built from, so `N = 2^order` factors.
///
/// It picks the curve `1 - (1 - z/N)^N`, with `z = x^2 / 2s^2`: Welsch's estimator as `N` grows,
/// and at a finite `N` Tukey's biweight generalised, which rejects a residual outright beyond
/// `s * sqrt(2N)`. Four puts that point at `5.66 * sigma`.
///
/// Raising it changes the curve, not only its accuracy, and the finite-difference step in
/// `tests/graph_derivatives_test.cpp` depends on it. See "Picking the kernel" in
/// `docs/design.md` for the family, what each order costs, and that coupling.
inline constexpr std::size_t order = 4;

/// @brief The redescending kernel `1 - (1 - z/N)^N`, with `z = x^2 / 2s^2` and `N = 2^order`.
///
/// Near zero it is `z`, so small residuals are penalised quadratically exactly as a plain squared
/// error would penalise them. It rises monotonically and saturates at 1, so a gross outlier
/// contributes a bounded amount and, more to the point, a vanishing derivative - it stops pulling
/// the estimate towards itself. Sigma is where the transition happens, and `sqrt(2N) * sigma` is
/// where the pull reaches exactly zero. See #order for the family this sits in.
///
/// It is `order` squarings - `a = a * a` in a loop, which the compiler unrolls and a constant
/// expression evaluates directly: no series, no range reduction, and nothing from `<cmath>`, so
/// the whole kernel is a handful of multiplications with nothing standing in for `std::exp`.
///
/// Past the rejection point the base turns negative, and an even power would send it climbing
/// again - so it is clamped, and the kernel saturates rather than turning back over. Multiplying
/// by zero rather than returning one keeps the result's type - a dual carries its indices - and
/// zeroes the value and the derivative together.
///
/// @param x The residual. A dual number here, so the derivative of the whole robustified error
/// comes out with it.
/// @param s The sigma for this dimension.
template <class X, class T>
constexpr auto radial(const X &x, const T &s) {
  constexpr auto count = static_cast<T>(std::size_t{1} << order);

  const auto scale = T{1} / (T{2} * count * s * s);
  const auto base = T{1} - scale * (x * x);

  auto folded = static_cast<T>(base) <= T{0} ? base * T{0} : base;
  for (std::size_t i = 0; i < order; ++i) {
    folded = folded * folded;
  }

  return T{1} - folded;
}

/// @brief The same radial shape, re-signed for a gradient rather than a residual.
///
/// A raw derivative carries both a direction and a magnitude. Sigma makes those scales comparable
/// across dimensions, while the radial kernel bounds the magnitude into [0, 1]: it is small near
/// zero and saturates for large values. The sign is then restored so the algorithm still knows
/// whether to move up or down.
///
/// This is the pattern used at each node dimension:
///   sign(g) * radial(g, sigma)
///
/// The magnitude is bounded, but the step itself is not; the algorithm may accumulate it further,
/// as `momentum` does. Without restoring the sign the update would always point in the positive
/// direction for negative gradients.
///
/// @param g The gradient component.
/// @param s The node's sigma for this dimension.
template <class T>
constexpr auto signed_radial(const T &g, const T &s) {
  const auto magnitude = radial(g, s);

  return g < T{0} ? -magnitude : magnitude;
}

/// @brief `signed_radial` applied component by component: a whole gradient, each component
/// bounded against the sigma of its own dimension.
///
/// @param g The gradient, one component per dimension.
/// @param s The sigmas, one per dimension.
template <class T, std::size_t N>
constexpr auto signed_radial(const std::array<T, N> &g, const std::array<T, N> &s)
    -> std::array<T, N> {
  std::array<T, N> out{};
  for (std::size_t i = 0; i < N; ++i) {
    out[i] = signed_radial(g[i], s[i]);
  }
  return out;
}

}  // namespace kernel
}  // namespace uzu

#endif  // UZU_OPTIMIZATION_KERNEL_HPP
