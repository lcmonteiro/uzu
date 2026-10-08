/// ===============================================================================================
/// @file
///
/// @brief What a graph's edges must satisfy, checked at compile time.
///
/// The node side - that no two nodes share a key - is a property of any keyed layout, so it is
/// `meta::keyed_layout::distinct_keys`. What is left here is the part only a graph has: edges,
/// and the keys they link. `graph` states both as `static_assert`s at the class, which is where a
/// broken declaration is reported.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP
#define UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP

#include <array>
#include <cstddef>

namespace uzu {
namespace impl {

/// @brief Whether every key one edge links is in @p Layout.
template <class Layout, std::size_t N>
constexpr auto resolves(const std::array<std::size_t, N> &linked) -> bool {
  for (const std::size_t key : linked) {
    if (!Layout::contains(key)) {
      return false;
    }
  }
  return true;
}

/// @brief Whether every edge links only keys in @p Layout.
///
/// Checked at the graph class rather than where an edge's nodes are looked up: the edges are all
/// in hand, so waiting until something instantiates the fit would only mean reporting a fixed
/// mistake later and behind a wall of tuple diagnostics.
///
/// @tparam Layout The node layout, a `meta::keyed_layout`.
/// @tparam Edges The edge entries, each carrying the `keys` it links.
template <class Layout, class... Edges>
inline constexpr bool edges_resolve = (true && ... && resolves<Layout>(Edges::keys));

}  // namespace impl
}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP
