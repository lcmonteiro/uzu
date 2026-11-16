/// ===============================================================================================
/// @file
///
/// @brief What a graph asks of what it is given, checked at compile time.
///
/// Three concepts say what a node, an edge and an algorithm must be - `uzu::node_like`,
/// `uzu::edge_like` and `uzu::algorithm_for` - so that a type can be checked on its own, and so
/// that `graph` can name the one that is wrong rather than fail somewhere inside itself. Beside
/// them is the part only a whole graph can check: that each edge links keys the nodes declare.
///
/// The node side of that - no two nodes sharing a key - is a property of any keyed layout, so it
/// is `meta::keyed_layout::distinct_keys`. `graph` states all of these as `static_assert`s at the
/// class, which is where a broken declaration is reported.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP
#define UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP

#include <array>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>

#include "uzu/helpers/ranges.hpp"
#include "uzu/optimization/graph/edge.hpp"
#include "uzu/optimization/graph/node.hpp"

namespace uzu {

/// @brief A node a graph can move: derived from `uzu::node` naming itself, with a `plus`.
///
/// Naming itself matters: the base reaches `plus` by casting itself to the class it was told is
/// derived from it, so `struct a : node<b, ...>` would call `b::plus` on an `a`. `plus` takes a
/// `point` - one delta per dimension - and returns the moved estimate.
template <class N>
concept node_like =
    requires {
      typename N::estimation_type;
      typename N::point;
      { N::dimension } -> std::convertible_to<std::size_t>;
    } && std::derived_from<N, node<N, typename N::estimation_type, N::dimension>> &&
    requires(N &n, const typename N::point &delta) {
      { n.plus(delta) } -> std::convertible_to<typename N::estimation_type>;
    };

/// @brief An edge a graph can score: derived from `uzu::edge` naming itself.
///
/// What its `error` takes depends on which nodes it is linked to, so that is checked where the
/// graph links it rather than here.
template <class E>
concept edge_like = requires {
  typename E::measurement_type;
  { E::dimension } -> std::convertible_to<std::size_t>;
} && std::derived_from<E, edge<E, typename E::measurement_type, E::dimension>>;

/// @brief An algorithm a graph of @p Width indices can run: a builder whose `build<Width>()`
/// returns its `instance<Width>`, which takes a `step<Index>` per index. See
/// `uzu/optimization/algorithm.hpp` for why it is two types.
template <class A, std::size_t Width>
concept algorithm_for =
    requires {
      typename A::value_type;
      typename A::template instance<Width>;
    } &&
    requires(
        const A &builder, typename A::template instance<Width> &instance,
        typename A::value_type slope) {
      { builder.template build<Width>() } -> std::same_as<typename A::template instance<Width>>;
      { instance.template step<0>(slope) } -> std::convertible_to<typename A::value_type>;
    };

namespace impl {

/// @brief Whether @p Edge's `error` takes one argument per element of @p Inputs, a `std::tuple`.
template <class Edge, class Inputs>
inline constexpr bool scores = false;

template <class Edge, class... Inputs>
inline constexpr bool scores<Edge, std::tuple<Inputs...>> =
    requires(const Edge &edge, const Inputs &...inputs) { edge.error(inputs...); };

/// @brief Every node, edge and the algorithm meet their concepts. Collected into arrays and
/// scanned rather than folded, for the reason `edges_resolve` gives below.
template <class... Nodes>
inline constexpr bool nodes_are_nodes =
    uzu::all_of(std::array<bool, sizeof...(Nodes)>{node_like<Nodes>...});

template <class... Edges>
inline constexpr bool edges_are_edges =
    uzu::all_of(std::array<bool, sizeof...(Edges)>{edge_like<Edges>...});

/// @brief Whether every key one edge links is in @p Layout.
template <class Layout, std::size_t N>
constexpr auto resolves(const std::array<std::size_t, N> &linked) -> bool {
  return uzu::all_of(linked, [](std::size_t key) { return Layout::contains(key); });
}

/// @brief Whether every edge links only keys in @p Layout.
///
/// Checked at the graph class rather than where an edge's nodes are looked up: the edges are all
/// in hand, so waiting until something instantiates the fit would only mean reporting a fixed
/// mistake later and behind a wall of tuple diagnostics.
///
/// @tparam Layout The node layout, a `meta::keyed_layout`.
/// @tparam Edges The edge entries, each carrying the `keys` it links.
///
/// Collected into an array and scanned, rather than folded: a fold over the edges expands into
/// an expression nested as deep as there are edges, and clang refuses one past 256 deep - so a
/// graph of 257 edges would not compile at all. An initializer list is flat at any length.
template <class Layout, class... Edges>
inline constexpr bool edges_resolve =
    uzu::all_of(std::array<bool, sizeof...(Edges)>{resolves<Layout>(Edges::keys)...});

}  // namespace impl
}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_CONTRACTS_HPP
