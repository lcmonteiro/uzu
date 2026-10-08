/// ===============================================================================================
/// @file
///
/// @brief The radial kernel, used both to robustify an edge's residual and to normalise a node's
/// gradient.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_KERNEL_HPP
#define UZU_OPTIMIZATION_KERNEL_HPP

#include <cstddef>

#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"

namespace uzu {
namespace kernel {

/// @brief How many squarings the kernel is built from, so `N = 2^order` factors.
///
/// This is the one number that picks the curve. The shape is
///
///     1 - (1 - z/N)^N          z = x^2 / 2s^2, clamped where the base turns negative
///
/// and `N` moves it across a family whose two ends are both standard robust estimators. As `N`
/// grows the curve tends to `1 - exp(-z)`, the Welsch estimator, which never quite stops pulling.
/// At a finite `N` it is Tukey's biweight generalised - the familiar cubic is this shape at
/// `N = 3` - and it reaches 1 exactly, at `x = s * sqrt(2N)`, beyond which a residual contributes
/// nothing at all and no derivative at all. That rejection point is the reason to prefer a small
/// `N`, not a concession for using one.
///
/// Four puts it at `5.66 * sigma`. Sigma already says where a residual stops being ordinary, so a
/// residual six sigma out is not an observation the fit should still be arguing with.
///
/// The cost side agrees. Each squaring carries all of a dual's derivatives with it, so the work
/// is linear in the order at both compile and run time:
///
///     order   rejection    fit of 1e6 passes   compiling the tests
///         4      5.66 s               0.06 s                8.9 s
///         8      22.6 s               0.10 s               10.9 s
///        16       362 s               0.20 s               14.8 s
///        20      1448 s               0.28 s               17.0 s
///
/// Raising it is one number, but it is a change of curve, not a refinement of one: the converged
/// error moves too, from 0.71664 at 4 to 0.71274 in the limit. `test.cpp`'s finite-difference
/// step is tied to this as well - see the note there.
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

}  // namespace kernel
}  // namespace uzu

#endif  // UZU_OPTIMIZATION_KERNEL_HPP
