/// ===============================================================================================
/// @file
///
/// @brief One expression over three duals, every operation in it, and each derivative held
/// against a finite difference. What it prints is compared byte for byte across library changes.
/// ===============================================================================================

#include <array>
#include <cmath>
#include <iostream>

#include "tests/helpers/run_time.hpp"
#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/number.hpp"
#include "uzu/foundation/dual/operations/cos.hpp"
#include "uzu/foundation/dual/operations/divides.hpp"
#include "uzu/foundation/dual/operations/exp.hpp"
#include "uzu/foundation/dual/operations/log.hpp"
#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"
#include "uzu/foundation/dual/operations/pow.hpp"
#include "uzu/foundation/dual/operations/sin.hpp"

namespace {

/// @brief The function under test, on plain doubles: what the duals below differentiate.
auto reference(const std::array<double, 3> &v) -> double {
  const double x = v[0];
  const double y = v[1];
  const double z = v[2];
  return std::cos(std::pow(x, std::log(y)) + 3.0 * std::exp(y - x)) / std::sin(z);
}

}  // namespace

int main() {
  auto b = dual::make_array<0, 3>(1.0);
  auto a = std::array{1.0, 2.0, 4.0};
  auto d = b + a;
  auto f = cos(pow(get<0>(d), log(get<1>(d))) + 3.0 * exp(get<1>(d) - get<0>(d))) / sin(get<2>(d));

  std::cout << "f     = " << f.value() << std::endl;
  std::cout << "df/dx = " << f.dvalue<0>() << std::endl;
  std::cout << "df/dy = " << f.dvalue<1>() << std::endl;
  std::cout << "df/dz = " << f.dvalue<2>() << std::endl;

  // Each derivative against a central difference of the same function on plain doubles, at the
  // point the duals were seeded at.
  using uzu::test::central_difference;
  using uzu::test::difference_tolerance;
  const std::array<double, 3> at{2.0, 3.0, 5.0};
  uzu::test::checks check;
  check.expect_close(f.value(), reference(at), "the value");
  check.expect_close(
      f.dvalue<0>(), central_difference(reference, at, 0), "df/dx", difference_tolerance);
  check.expect_close(
      f.dvalue<1>(), central_difference(reference, at, 1), "df/dy", difference_tolerance);
  check.expect_close(
      f.dvalue<2>(), central_difference(reference, at, 2), "df/dz", difference_tolerance);
  return check.result();
}
