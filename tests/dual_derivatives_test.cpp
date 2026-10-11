/// ===============================================================================================
/// @file
///
/// @brief `dual::accumulate`: adding a number's derivatives into a dense array, slot by index.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include <array>

#include "tests/helpers/compile_time.hpp"
#include "uzu/foundation/dual/derivatives.hpp"
#include "uzu/foundation/dual/number.hpp"

namespace {

/// @brief Two numbers over different, overlapping indices, accumulated into four slots.
constexpr auto accumulated() -> std::array<double, 4> {
  auto slots = std::array<double, 4>{};
  dual::accumulate(dual::number<double, 1, 3>{5.0, 2.0}, slots);
  dual::accumulate(dual::number<double, 3>{7.0, 0.5}, slots);
  return slots;
}

constexpr auto slots = accumulated();

static_assert(slots[0] == 0.0, "an index no number carries stays zero");
static_assert(slots[1] == 2.0, "an index one number carries takes its derivative");
static_assert(slots[2] == 0.0, "the slots between are left alone");
static_assert(slots[3] == 2.5, "an index both carry takes the sum of their derivatives");

/// @brief A number with no indices adds nothing.
constexpr auto untouched() -> std::array<double, 2> {
  auto none = std::array<double, 2>{1.0, -1.0};
  dual::accumulate(dual::number<double>{9.0}, none);
  return none;
}

static_assert(untouched()[0] == 1.0 && untouched()[1] == -1.0, "no indices, no change");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("dual_derivatives");
}
