/// ===============================================================================================
/// @file
///
/// @brief `uzu::tuple`: what it holds, how it is read, and that it stays flat at any length.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include "uzu/helpers/tuple.hpp"

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "tests/helpers/compile_time.hpp"

namespace {

// ---------------------------------------------------------------------------------------------
// Holding and reading.
// ---------------------------------------------------------------------------------------------

constexpr auto three = uzu::tuple{1, 2.5, 'c'};

static_assert(
    std::is_same_v<std::remove_const_t<decltype(three)>, uzu::tuple<int, double, char>>,
    "the element types are deduced from the values");
static_assert(three.size() == 3 && std::tuple_size_v<decltype(three)> == 3, "it knows its size");
static_assert(
    uzu::get<0>(three) == 1 && uzu::get<1>(three) == 2.5 && uzu::get<2>(three) == 'c',
    "get<I> reads the element at I");
static_assert(
    std::is_same_v<std::tuple_element_t<1, uzu::tuple<int, double, char>>, double>,
    "tuple_element names the element's type");

/// @brief Two elements of the same type are still two elements.
constexpr auto twins = uzu::tuple<int, int>{4, 7};
static_assert(uzu::get<0>(twins) == 4 && uzu::get<1>(twins) == 7, "same type, own values");

/// @brief An empty tuple, and a default-constructed one, are tuples too.
static_assert(uzu::tuple<>{}.size() == 0, "an empty tuple holds nothing");
static_assert(uzu::get<1>(uzu::tuple<int, double>{}) == 0.0, "a default tuple holds zeros");

/// @brief Writes through get<I>, and moves out of an rvalue one.
constexpr auto written() {
  auto t = uzu::tuple<int, double>{1, 2.0};
  uzu::get<0>(t) = 10;
  uzu::get<1>(t) *= 3.0;
  return t;
}
static_assert(uzu::get<0>(written()) == 10 && uzu::get<1>(written()) == 6.0, "get<I> writes");
static_assert(std::is_same_v<decltype(uzu::get<0>(written())), int &&>, "and moves from rvalues");

/// @brief Structured bindings, which go through tuple_size, tuple_element and get.
constexpr auto bound() {
  const auto [a, b, c] = uzu::tuple{3, 4.5, 'z'};
  return a + b + (c == 'z' ? 1.0 : 0.0);
}
static_assert(bound() == 8.5, "it unpacks into structured bindings");

// ---------------------------------------------------------------------------------------------
// apply and for_each.
// ---------------------------------------------------------------------------------------------

static_assert(
    uzu::apply([](int a, double b, char c) { return a + b + c; }, three) == 1 + 2.5 + 'c',
    "apply calls with every element as an argument");

/// @brief for_each visits first to last.
constexpr auto visited() {
  std::array<int, 4> order{};
  std::size_t at = 0;
  uzu::for_each(uzu::tuple{7, 8, 9, 10}, [&](int x) { order[at++] = x; });
  return order;
}
static_assert(visited() == std::array<int, 4>{7, 8, 9, 10}, "for_each goes in order");

/// @brief A result type whose comma operator is deleted. for_each throws away what `f` returns,
/// and must do so with the built-in comma: `(f(x), 0)` would pick this overload instead, and
/// fail to compile. A comma that is overloaded rather than deleted would be worse - it would be
/// called, and could do anything.
struct no_comma {
  int value;
  friend auto operator,(no_comma, int) -> int = delete;
};

constexpr auto discarded() {
  int sum = 0;
  uzu::for_each(uzu::tuple{1, 2, 3}, [&](int x) {
    sum += x;
    return no_comma{x};
  });
  return sum;
}
static_assert(discarded() == 6, "for_each ignores what f returns, whatever its comma does");

// ---------------------------------------------------------------------------------------------
// Length.
//
// A tuple of 300 distinct types, built and read through every route. With std::tuple this alone
// takes seconds of the compiler's time; and for_each over it would be a fold of 300, past the
// 256 clang allows, if it were written as one.
// ---------------------------------------------------------------------------------------------

template <std::size_t I>
struct item {
  std::size_t value;
};

template <std::size_t... Is>
constexpr auto long_sums(std::index_sequence<Is...>) {
  const auto t = uzu::tuple<item<Is>...>{item<Is>{Is}...};

  const std::array<std::size_t, sizeof...(Is)> each{uzu::get<Is>(t).value...};
  std::size_t by_get = 0;
  for (const auto x : each) {
    by_get += x;
  }

  std::size_t by_each = 0;
  uzu::for_each(t, [&](const auto &x) { by_each += x.value; });

  const auto by_apply = uzu::apply([](const auto &...xs) { return sizeof...(xs); }, t);

  return std::array{by_get, by_each, by_apply};
}

constexpr std::size_t kLong = 300;
constexpr auto sums = long_sums(std::make_index_sequence<kLong>{});
constexpr auto expected = kLong * (kLong - 1) / 2;

static_assert(sums[0] == expected, "300 elements, each read by get<I>");
static_assert(sums[1] == expected, "and by for_each");
static_assert(sums[2] == kLong, "and all 300 are handed to apply");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("tuple");
}
