/// ===============================================================================================
/// @file
///
/// @brief `uzu::accumulate`, `uzu::any_of` and `uzu::all_of`: the three loops the library keeps
/// writing over a `std::array` of values computed from a pack.
///
/// The pattern they serve is a pack turned into an array and then walked:
///
///     static constexpr std::size_t width = uzu::accumulate(widths, std::size_t{0});
///     inline constexpr bool ok = uzu::all_of(std::array<bool, N>{check(Ts)...});
///
/// rather than folded, `(Ts::width + ... + 0)`. A fold nests as deep as the pack is long, and
/// clang refuses one past 256 elements; an array is flat at any length.
///
/// C++20 has `constexpr` versions of all three in `<numeric>` and `<algorithm>`. These exist
/// anyway for the reason `dual/number.hpp` keeps a hand-written search instead of `std::find`:
/// measured there, the standard one cost 5% more of the compiler's front end, for no difference
/// in what it computes. These need nothing but `begin` and `end` on what they are given.
/// ===============================================================================================
#ifndef UZU_HELPERS_RANGES_HPP
#define UZU_HELPERS_RANGES_HPP

#include <utility>

namespace uzu {
namespace impl {

/// @brief The predicate `any_of` and `all_of` use when given none: the value itself, as a bool.
struct truthy {
  template <class T>
  constexpr auto operator()(const T &value) const -> bool {
    return static_cast<bool>(value);
  }
};

}  // namespace impl

/// @brief @p init plus every element of @p range, added left to right.
template <class Range, class T>
constexpr auto accumulate(const Range &range, T init) -> T {
  for (const auto &value : range) {
    init = std::move(init) + value;
  }
  return init;
}

/// @brief Whether @p pred holds for at least one element of @p range. Stops at the first that
/// does; false for an empty range.
template <class Range, class Pred = impl::truthy>
constexpr auto any_of(const Range &range, Pred pred = {}) -> bool {
  for (const auto &value : range) {
    if (pred(value)) {
      return true;
    }
  }
  return false;
}

/// @brief Whether @p pred holds for every element of @p range. Stops at the first that does
/// not; true for an empty range.
template <class Range, class Pred = impl::truthy>
constexpr auto all_of(const Range &range, Pred pred = {}) -> bool {
  for (const auto &value : range) {
    if (!pred(value)) {
      return false;
    }
  }
  return true;
}

}  // namespace uzu

#endif  // UZU_HELPERS_RANGES_HPP
