/// ===============================================================================================
/// @file
///
/// @brief The duals over arrays: element-wise operations, broadcasting, a fold, and printing.
/// Each result is checked; what it prints is compared byte for byte across library changes.
/// ===============================================================================================

#include <array>
#include <cmath>
#include <tuple>

#include "tests/helpers/run_time.hpp"
#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/functional.hpp"
#include "uzu/foundation/dual/operations/cos.hpp"
#include "uzu/foundation/dual/operations/divides.hpp"
#include "uzu/foundation/dual/operations/exp.hpp"
#include "uzu/foundation/dual/operations/log.hpp"
#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"
#include "uzu/foundation/dual/operations/pow.hpp"
#include "uzu/foundation/dual/operations/sin.hpp"
#include "uzu/foundation/dual/operations/sqrt.hpp"
#include "uzu/foundation/dual/print.hpp"

namespace {

/// @brief The element-wise function `d` applies, on a plain double, with `a` held constant.
auto chain(double a, double t) -> double {
  return std::cos(std::sin(std::log(std::exp(a / t + a))));
}

}  // namespace

int main() {
  auto b = dual::make_array<0>(1.0, 2.0, 3.);
  auto a = std::array{1.0, 2.0, 4.0};
  auto n = dual::number<double, 3>{2.0};
  auto d = cos(sin(log(exp(a / b + a))));
  // Checked but not printed: what this program prints stays fixed across library changes.
  auto e = cos(sin(n)) * 4.0;
  auto f = dual::array(std::tie(n, n));
  // A fold over the elements: dual::product is the Cartesian product, not this.
  auto g = dual::apply([](const auto &...x) { return ((x + 1.0) * ...); }, b);

  dual::print(pow(b, 0.5));
  dual::print(sqrt(b));
  dual::print(d);
  dual::print(f);
  dual::print(g);

  using uzu::test::central_difference;
  using uzu::test::difference_tolerance;
  uzu::test::checks check;

  // pow(b, 0.5) and sqrt(b) are the same function, with derivative 1 / (2 sqrt(b)).
  const auto roots = pow(b, 0.5);
  const auto square_roots = sqrt(b);
  check.expect_close(get<1>(roots).value(), std::sqrt(2.0), "pow(b, 0.5)");
  check.expect_close(get<1>(roots).dvalue<1>(), 0.5 / std::sqrt(2.0), "d pow(b, 0.5)");
  check.expect_close(get<2>(square_roots).value(), std::sqrt(3.0), "sqrt(b)");
  check.expect_close(get<2>(square_roots).dvalue<2>(), 0.5 / std::sqrt(3.0), "d sqrt(b)");

  // d: element i depends on b_i alone, so each carries one derivative, at its own index.
  const auto at_0 = [&](const std::array<double, 1> &t) { return chain(a[0], t[0]); };
  const auto at_2 = [&](const std::array<double, 1> &t) { return chain(a[2], t[0]); };
  check.expect_close(get<0>(d).value(), chain(a[0], 1.0), "d[0]");
  check.expect_close(
      get<0>(d).dvalue<0>(),
      central_difference(at_0, std::array{1.0}, 0),
      "d d[0] / d b0",
      difference_tolerance);
  check.expect_close(
      get<2>(d).dvalue<2>(),
      central_difference(at_2, std::array{3.0}, 0),
      "d d[2] / d b2",
      difference_tolerance);

  // e = 4 cos(sin(n)), so de/dn = -4 sin(sin(n)) cos(n).
  check.expect_close(e.value(), 4.0 * std::cos(std::sin(2.0)), "e");
  check.expect_close(e.dvalue<3>(), -4.0 * std::sin(std::sin(2.0)) * std::cos(2.0), "de/dn");

  // f holds n twice, by value.
  check.expect(get<0>(f).value() == 2.0 && get<1>(f).dvalue<3>() == 1.0, "f is n, twice");

  // g = (b0 + 1)(b1 + 1)(b2 + 1) = 2 * 3 * 4, and each derivative is the product of the others.
  check.expect_close(g.value(), 24.0, "g");
  check.expect_close(g.dvalue<0>(), 12.0, "dg/db0");
  check.expect_close(g.dvalue<1>(), 8.0, "dg/db1");
  check.expect_close(g.dvalue<2>(), 6.0, "dg/db2");

  return check.result();
}
