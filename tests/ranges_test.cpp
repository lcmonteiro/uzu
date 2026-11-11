/// ===============================================================================================
/// @file
///
/// @brief `uzu::accumulate`, `uzu::any_of` and `uzu::all_of`.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include "uzu/helpers/ranges.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include "tests/helpers/compile_time.hpp"

namespace {

constexpr std::array<std::size_t, 4> widths{2, 3, 1, 4};
constexpr std::array<std::size_t, 0> none{};

// ---------------------------------------------------------------------------------------------
// accumulate
// ---------------------------------------------------------------------------------------------

static_assert(uzu::accumulate(widths, std::size_t{0}) == 10, "every element is added");
static_assert(uzu::accumulate(widths, std::size_t{5}) == 15, "on top of the initial value");
static_assert(uzu::accumulate(none, std::size_t{7}) == 7, "an empty range leaves it as it was");
static_assert(uzu::accumulate(std::array{0.5, 0.25}, 1.0) == 1.75, "in the type of the init");

/// @brief Left to right: with a non-commutative `+`, the order shows in the result.
struct digits {
  int value;
  constexpr auto operator+(int d) const -> digits {
    return {value * 10 + d};
  }
};
static_assert(uzu::accumulate(std::array{1, 2, 3}, digits{0}).value == 123, "first to last");

// ---------------------------------------------------------------------------------------------
// any_of / all_of
// ---------------------------------------------------------------------------------------------

constexpr auto is_even = [](std::size_t x) { return x % 2 == 0; };
constexpr auto is_small = [](std::size_t x) { return x < 5; };

static_assert(uzu::any_of(widths, is_even), "any_of: one element is enough");
static_assert(!uzu::any_of(widths, [](std::size_t x) { return x > 4; }), "and none is not");
static_assert(uzu::all_of(widths, is_small), "all_of: every element");
static_assert(!uzu::all_of(widths, is_even), "and one exception is enough to fail");

static_assert(!uzu::any_of(none, is_even), "nothing satisfies any_of on an empty range");
static_assert(uzu::all_of(none, is_even), "everything satisfies all_of on an empty range");

static_assert(
    uzu::all_of(std::array{true, true}) && !uzu::all_of(std::array{true, false}),
    "without a predicate, the elements are the answers");
static_assert(
    uzu::any_of(std::array{false, true}) && !uzu::any_of(std::array{false, false}),
    "for any_of too");

/// @brief Both stop at the element that decides: a predicate that counts its calls shows it.
constexpr auto calls_until_decided() {
  int any_calls = 0;
  int all_calls = 0;
  const std::array<int, 5> xs{1, 2, 3, 4, 5};
  uzu::any_of(xs, [&](int x) { return ++any_calls, x == 2; });
  uzu::all_of(xs, [&](int x) { return ++all_calls, x < 3; });
  return std::pair{any_calls, all_calls};
}
static_assert(calls_until_decided() == std::pair{2, 3}, "both stop as soon as they know");

// ---------------------------------------------------------------------------------------------
// Length: what they replace is a fold, which clang refuses past 256 elements.
// ---------------------------------------------------------------------------------------------

template <std::size_t... Is>
constexpr auto long_checks(std::index_sequence<Is...>) {
  const std::array<std::size_t, sizeof...(Is)> xs{Is...};
  const std::array<bool, sizeof...(Is)> checks{(Is < sizeof...(Is))...};
  return std::array{
      uzu::accumulate(xs, std::size_t{0}),
      std::size_t{uzu::all_of(checks)},
      std::size_t{uzu::any_of(xs, [](std::size_t x) { return x == 299; })}};
}

constexpr auto long_results = long_checks(std::make_index_sequence<300>{});
static_assert(long_results[0] == 300 * 299 / 2, "300 values summed");
static_assert(long_results[1] == 1 && long_results[2] == 1, "and checked");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("ranges");
}
