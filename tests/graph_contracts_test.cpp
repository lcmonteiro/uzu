/// ===============================================================================================
/// @file
///
/// @brief The graph's contracts: distinct keys, and edges that link only declared keys.
///
/// The checks are free functions over the keys, so they are asserted here on plain arrays, both
/// ways round. The failing side cannot be shown through `graph` itself: a translation unit that
/// declares a repeated key, or links an undeclared one, does not compile.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include "uzu/optimization/graph_contracts.hpp"

#include <array>
#include <cstddef>

#include "tests/helpers/compile_time.hpp"

namespace {

using keys = std::array<std::size_t, 3>;

/// @brief Stands in for an edge entry: only its `keys` are read.
template <std::size_t... Keys>
struct link_stub {
  static constexpr std::array<std::size_t, sizeof...(Keys)> keys{Keys...};
};

constexpr keys declared{4, 7, 9};

// ---------------------------------------------------------------------------------------------
// Finding a key.
// ---------------------------------------------------------------------------------------------

static_assert(uzu::impl::find_key(declared, 4) == 0, "the first key is entry 0");
static_assert(uzu::impl::find_key(declared, 9) == 2, "the last key is entry 2");
static_assert(uzu::impl::find_key(declared, 5) == 3, "an undeclared key is one past the end");

// ---------------------------------------------------------------------------------------------
// Distinct keys.
// ---------------------------------------------------------------------------------------------

static_assert(uzu::impl::distinct_keys(declared), "distinct keys pass");
static_assert(!uzu::impl::distinct_keys(keys{4, 7, 4}), "a repeated key fails");
static_assert(uzu::impl::distinct_keys(std::array<std::size_t, 0>{}), "no nodes is no repeat");

// ---------------------------------------------------------------------------------------------
// Edges resolve.
// ---------------------------------------------------------------------------------------------

static_assert(
    uzu::impl::edges_resolve<link_stub<4>, link_stub<7, 9>>(declared),
    "edges over declared keys pass");
static_assert(
    !uzu::impl::edges_resolve<link_stub<4>, link_stub<7, 5>>(declared),
    "one undeclared key in any edge fails");
static_assert(uzu::impl::edges_resolve<>(declared), "no edges resolve trivially");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("graph_contracts");
}
