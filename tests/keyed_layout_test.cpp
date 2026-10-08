/// ===============================================================================================
/// @file
///
/// @brief The keyed layout on its own: lookups, offsets, width, and the distinct-keys check.
///
/// The layout knows nothing of graphs, so it is exercised here with bare stand-ins carrying only
/// a key and a width. The graph's own tests then hold that it lays its nodes out the same way.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include "uzu/foundation/meta/keyed_layout.hpp"

#include <cstddef>

#include "tests/helpers/compile_time.hpp"

namespace {

/// @brief Stands in for an entry: only its `key` and `width` are read.
template <std::size_t Key, std::size_t Width>
struct block {
  static constexpr std::size_t key = Key;
  static constexpr std::size_t width = Width;
};

using layout = uzu::meta::keyed_layout<block<4, 2>, block<7, 3>, block<9, 1>>;

// ---------------------------------------------------------------------------------------------
// Shape.
// ---------------------------------------------------------------------------------------------

static_assert(layout::size == 3, "one block per entry");
static_assert(layout::width == 6, "the blocks laid end to end, and nothing between them");

// ---------------------------------------------------------------------------------------------
// Lookups.
// ---------------------------------------------------------------------------------------------

static_assert(layout::index_of(4) == 0, "the first key is entry 0");
static_assert(layout::index_of(9) == 2, "the last key is entry 2");
static_assert(layout::index_of(5) == layout::size, "an absent key is one past the last entry");

static_assert(layout::contains(7), "a declared key is contained");
static_assert(!layout::contains(5), "an absent key is not");

static_assert(layout::offset_of(4) == 0, "the first block starts at 0");
static_assert(layout::offset_of(7) == 2, "each block starts where the one before ends");
static_assert(layout::offset_of(9) == 5, "offsets accumulate in declaration order");
static_assert(layout::offset_of(5) == layout::width, "an absent key is past the last block");

static_assert(layout::entry_at<1>::key == 7, "entries keep their declaration order");

// ---------------------------------------------------------------------------------------------
// Distinct keys.
// ---------------------------------------------------------------------------------------------

static_assert(layout::distinct_keys, "distinct keys pass");
static_assert(
    !uzu::meta::keyed_layout<block<4, 2>, block<7, 1>, block<4, 3>>::distinct_keys,
    "a repeated key fails, wherever the repeat is");

using empty = uzu::meta::keyed_layout<>;

static_assert(empty::distinct_keys, "no entries is no repeat");
static_assert(empty::width == 0, "no entries take no indices");
static_assert(!empty::contains(0), "and contain nothing");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("keyed_layout");
}
