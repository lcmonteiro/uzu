/// ===============================================================================================
/// @file
///
/// @brief What every run-time test in this folder needs: checks that record a failure and carry
/// on, and a central finite difference to hold a derivative against.
///
/// A failure is reported on `stderr`, so that a passing test's `stdout` is exactly what it prints
/// on purpose - the dual tests' output is compared byte for byte across library changes.
/// ===============================================================================================
#ifndef UZU_TESTS_HELPERS_RUN_TIME_HPP
#define UZU_TESTS_HELPERS_RUN_TIME_HPP

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>

namespace uzu::test {

/// @brief A tally of checks. Each failing one is reported as it happens; `result` is the exit code.
class checks {
 public:
  /// @brief Records @p what as failed unless @p condition holds.
  auto expect(bool condition, const char *what) -> void {
    if (!condition) {
      std::fprintf(stderr, "FAILED: %s\n", what);
      held_ = false;
    }
  }

  /// @brief Whether @p actual is within @p tolerance of @p wanted, relative to `max(1, |wanted|)`.
  auto expect_close(double actual, double wanted, const char *what, double tolerance = 1e-12)
      -> void {
    const double scale = std::fmax(1.0, std::fabs(wanted));
    if (std::fabs(actual - wanted) > tolerance * scale) {
      std::fprintf(stderr, "FAILED: %s (got %.17g, wanted %.17g)\n", what, actual, wanted);
      held_ = false;
    }
  }

  /// @brief The exit code: 0 if every check held, 1 otherwise.
  auto result() const -> int {
    return held_ ? 0 : 1;
  }

 private:
  bool held_ = true;
};

/// @brief The tolerance a derivative is held to against `central_difference`: its error is
/// `O(h^2)` from the step and `O(eps / h)` from rounding, both well under this at `h = 1e-6`.
inline constexpr double difference_tolerance = 1e-7;

/// @brief `df/dx_i` at @p x, by central difference: `(f(x + h e_i) - f(x - h e_i)) / 2h`.
/// @param f A function of @p N plain doubles, taking them as a `std::array`.
template <class F, std::size_t N>
auto central_difference(F f, const std::array<double, N> &x, std::size_t i, double h = 1e-6)
    -> double {
  auto up = x;
  auto down = x;
  up[i] += h;
  down[i] -= h;
  return (f(up) - f(down)) / (2.0 * h);
}

}  // namespace uzu::test

#endif  // UZU_TESTS_HELPERS_RUN_TIME_HPP
