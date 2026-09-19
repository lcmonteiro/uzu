/// ===============================================================================================
/// @file
///
/// @brief What every compile-time test in this folder needs.
///
/// The comparisons are here rather than in `<cmath>` because `std::fabs` cannot run in a constant
/// expression, and every assertion in this suite is a `static_assert`. `passed` is what a test
/// binary does at run time: by the time it runs, the compiler has already evaluated every fit and
/// every property, so all that is left is to say so.
/// ===============================================================================================
#ifndef UZU_TESTS_HELPERS_COMPILE_TIME_HPP
#define UZU_TESTS_HELPERS_COMPILE_TIME_HPP

#include <cstdio>

namespace uzu::test {

/// @brief `std::fabs` is not usable at compile time, and this is all of it that is needed.
constexpr auto magnitude(double x) -> double {
  return x < 0.0 ? -x : x;
}

/// @brief Whether two values agree to within @p tolerance.
constexpr auto close(double a, double b, double tolerance) -> bool {
  return magnitude(a - b) <= tolerance;
}

/// @brief Reports that a translation unit's assertions held. Reaching it *is* the pass.
inline auto passed(const char *what) -> int {
  std::printf("%s: all passed, at compile time\n", what);
  return 0;
}

}  // namespace uzu::test

#endif  // UZU_TESTS_HELPERS_COMPILE_TIME_HPP
