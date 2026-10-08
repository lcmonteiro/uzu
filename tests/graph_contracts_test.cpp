/// ===============================================================================================
/// @file
///
/// @brief The graph's contracts: edges that link only keys the node layout declares.
///
/// The failing side cannot be shown through `graph` itself - a translation unit that links an
/// undeclared key does not compile - so the predicate is asserted here directly, both ways round.
/// The node side, distinct keys, belongs to the layout and is tested with it.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include <array>
#include <cstddef>

#include "tests/helpers/compile_time.hpp"
#include "uzu/foundation/meta/keyed_layout.hpp"
#include "uzu/optimization/graph/contracts.hpp"

namespace {

/// @brief Stands in for a node entry: only its `key` and `width` are read.
template <std::size_t Key>
struct node_stub {
  static constexpr std::size_t key = Key;
  static constexpr std::size_t width = 2;
};

/// @brief Stands in for an edge entry: only its `keys` are read.
template <std::size_t... Keys>
struct edge_stub {
  static constexpr std::array<std::size_t, sizeof...(Keys)> keys{Keys...};
};

using declared = uzu::meta::keyed_layout<node_stub<4>, node_stub<7>, node_stub<9>>;

static_assert(
    uzu::impl::edges_resolve<declared, edge_stub<4>, edge_stub<7, 9>>,
    "edges over declared keys pass");
static_assert(
    !uzu::impl::edges_resolve<declared, edge_stub<4>, edge_stub<7, 5>>,
    "one undeclared key in any edge fails");
static_assert(uzu::impl::edges_resolve<declared>, "no edges resolve trivially");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("graph_contracts");
}
