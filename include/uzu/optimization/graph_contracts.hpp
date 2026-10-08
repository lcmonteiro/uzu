/// ===============================================================================================
/// @file
///
/// @brief What a graph's nodes and edges must satisfy, checked at compile time.
///
/// The checks are free functions over the keys rather than members of `graph`, so that they can
/// be read, and tested, on their own. `graph` states them as `static_assert`s at the class, which
/// is where a broken declaration is reported.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP
#define UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP

#include <array>
#include <cstddef>

namespace uzu {
namespace impl {

/// @brief Which entry of @p keys is @p wanted, or `N` if none is.
///
/// The one scan over the keys. Everything that has to find a node - the layout, the lookups, and
/// the check below that every edge names something - goes through here rather than repeating it.
template <std::size_t N>
constexpr auto find_key(const std::array<std::size_t, N> &keys, std::size_t wanted) -> std::size_t {
  for (std::size_t i = 0; i < N; ++i) {
    if (keys[i] == wanted) {
      return i;
    }
  }
  return N;
}

/// @brief Whether every declared node carries a distinct key.
///
/// Without this a repeated key is silent and wrong rather than an error: the lookups take the
/// first node that matches, so both would read the same gradient and step identically, while the
/// second one's block sat in the layout with nothing referring to it.
template <std::size_t N>
constexpr auto distinct_keys(const std::array<std::size_t, N> &keys) -> bool {
  for (std::size_t i = 0; i < N; ++i) {
    for (std::size_t j = i + 1; j < N; ++j) {
      if (keys[i] == keys[j]) {
        return false;
      }
    }
  }
  return true;
}

/// @brief Whether every key one edge links is declared.
template <std::size_t N, std::size_t M>
constexpr auto resolves(
    const std::array<std::size_t, N> &keys, const std::array<std::size_t, M> &named) -> bool {
  for (const std::size_t wanted : named) {
    if (find_key(keys, wanted) == N) {
      return false;
    }
  }
  return true;
}

/// @brief Whether every edge links only declared keys.
///
/// Checked at the graph class rather than where an edge's nodes are looked up: the edges are all
/// in hand, so waiting until something instantiates the fit would only mean reporting a fixed
/// mistake later and behind a wall of tuple diagnostics.
///
/// @tparam Ls The edge entries, each carrying the `keys` it links.
template <class... Ls, std::size_t N>
constexpr auto edges_resolve(const std::array<std::size_t, N> &keys) -> bool {
  return (true && ... && resolves(keys, Ls::keys));
}

}  // namespace impl
}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP
