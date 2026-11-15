/// ===============================================================================================
/// @file
///
/// @brief `pow` at the points where the textbook derivative formula breaks down.
///
/// `d/dx x^y = y x^(y-1)` and `d/dy x^y = x^y ln(x)` both multiply zero by infinity somewhere, and
/// the derivative has to come out as the limit rather than as NaN. `std::pow` cannot run at compile
/// time, so unlike most of the suite this one is checked when it runs.
/// ===============================================================================================

#include <cmath>
#include <cstdio>

#include "tests/helpers/compile_time.hpp"
#include "uzu/foundation/dual/number.hpp"
#include "uzu/foundation/dual/operations/pow.hpp"

namespace {

using base = dual::number<double, 0>;
using exponent = dual::number<double, 1>;

bool held = true;

auto expect(bool condition, const char *what) -> void {
  if (!condition) {
    std::printf("FAILED: %s\n", what);
    held = false;
  }
}

auto expect_close(double actual, double wanted, const char *what) -> void {
  expect(uzu::test::close(actual, wanted, 1e-12), what);
}

}  // namespace

auto main() -> int {
  // An ordinary point, against the formulas written out.
  const auto ordinary = pow(base{2.0}, exponent{3.0});
  expect_close(ordinary.value(), 8.0, "2^3");
  expect_close(ordinary.dvalue<0>(), 12.0, "d/dx x^y = y x^(y-1)");
  expect_close(ordinary.dvalue<1>(), 8.0 * std::log(2.0), "d/dy x^y = x^y ln(x)");

  // A zero base: ln(0) must not reach the result.
  const auto at_zero = pow(base{0.0}, exponent{2.0});
  expect_close(at_zero.dvalue<0>(), 0.0, "d/dx x^2 at 0");
  expect_close(at_zero.dvalue<1>(), 0.0, "d/dy 0^y is the limit, zero");

  // A negative base under an exponent that does not vary is an ordinary power.
  const auto negative = pow(base{-2.0}, exponent{3.0, 0.0});
  expect_close(negative.value(), -8.0, "(-2)^3");
  expect_close(negative.dvalue<0>(), 12.0, "d/dx x^3 at -2");
  expect_close(negative.dvalue<1>(), 0.0, "a constant exponent contributes nothing");

  // The same, with the exponent a plain scalar and with both sharing an index.
  expect_close(pow(base{-2.0}, 3.0).dvalue<0>(), 12.0, "d/dx x^3 at -2, scalar exponent");
  expect_close(pow(base{-2.0}, base{3.0, 0.0}).dvalue<0>(), 12.0, "d/dx x^3 at -2, shared index");

  // x^0 is the constant one, at zero too.
  expect_close(pow(base{0.0}, 0.0).dvalue<0>(), 0.0, "d/dx x^0 at 0");

  // A negative base under a varying exponent has no real derivative.
  expect(std::isnan(pow(base{-2.0}, exponent{3.0}).dvalue<1>()), "d/dy (-2)^y stays NaN");

  if (!held) {
    return 1;
  }
  std::printf("dual_pow: all passed\n");
  return 0;
}
