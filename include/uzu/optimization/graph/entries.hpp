/// ===============================================================================================
/// @file
///
/// @brief How nodes and edges enter a graph: under a key, and over the keys they link.
///
/// `nodes{key<1>(n1)}` keys a node; `edges{link<1, 2>(e1)}` links an edge over the keys it
/// constrains. The entries record what was said - a target, and the keys - and nothing else; the
/// graph works out the layout and the lookups from them.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_ENTRIES_HPP
#define UZU_OPTIMIZATION_GRAPH_ENTRIES_HPP

#include <array>
#include <cstddef>
#include <utility>

namespace uzu {
namespace impl {

/// @brief A node in the graph's node list: an estimate, under a key.
///
/// Supplies `key` and `width`, which is all a `meta::keyed_layout` asks of an entry: the node's
/// dimension is the width of the block it takes up.
template <std::size_t Key, class Node>
struct node_entry {
  static constexpr std::size_t key = Key;
  static constexpr std::size_t width = Node::dimension;

  Node *target;
};

/// @brief One element of a `pack`: a value, tagged with its position so that two elements of the
/// same type are still two different bases.
template <std::size_t I, class T>
struct leaf {
  T value;
};

template <class Indices, class... Ts>
struct pack_base;

template <std::size_t... Is, class... Ts>
struct pack_base<std::index_sequence<Is...>, Ts...> : leaf<Is, Ts>... {
  constexpr explicit pack_base(const Ts &...values) : leaf<Is, Ts>{values}... {
  }
};

/// @brief A flat, fixed list of values: what `nodes` and `edges` hold, and the graph after them.
///
/// It replaces a `std::tuple`, which is what made a large graph slow to compile. A tuple is a
/// recursion, one level per element, and every `std::get<I>` or `std::tuple_element_t<I>` walks
/// it, so a graph that looked up each of its E edges in a tuple of E did E^2 work in the
/// compiler; its constructors also check their constraints across the whole pack. Here every
/// element is a direct base, built in one pack expansion, and the graph never looks one up by
/// position: it expands its own type list and the indices in lockstep, `leaf<Is, Es>...`, so each
/// element is reached by naming its base. On a pose graph of 5 edges per node, with GCC at -O3,
/// that took a 128-node graph from 614 s to 242 s to compile, and a 64-node one from 90 s to 73 s;
/// below 32 nodes the difference is within noise, since the cost it removes is the one that
/// grows with the square of the graph.
template <class... Ts>
struct pack : pack_base<std::index_sequence_for<Ts...>, Ts...> {
  using pack_base<std::index_sequence_for<Ts...>, Ts...>::pack_base;
};

/// @brief The element at @p I, found by naming its base. Deducing `T` from the base is a lookup
/// the compiler does directly, without instantiating anything per element.
template <std::size_t I, class T>
constexpr auto element(const leaf<I, T> &l) -> const T & {
  return l.value;
}

/// @brief An edge in the graph's edge list, over the keys it links.
template <class Edge, std::size_t... Keys>
struct edge_entry {
  using target_type = Edge;

  static constexpr std::size_t arity = sizeof...(Keys);
  static constexpr std::array<std::size_t, arity> keys{Keys...};

  Edge *target;
};

}  // namespace impl

/// @brief Puts a node in the graph under a key.
///
/// Named `key` rather than `node` because `node` is already the base a node
/// derives from, and a class template and a function template cannot share a
/// name - both GCC and clang reject it outright. A function rather than a
/// class so that the key can be given and the node type deduced; class
/// template argument deduction is all or nothing.
template <std::size_t Key, class Node>
constexpr auto key(Node &target) {
  return impl::node_entry<Key, Node>{&target};
}

/// @brief Puts an edge in the graph, over the keys it constrains.
///
/// Named `link` rather than `edge` for the same reason `key` is not `node`: `edge` is the base an
/// edge derives from. What it produces is an `edge_entry`, named for what it holds.
template <std::size_t... Keys, class Edge>
constexpr auto link(Edge &target) {
  return impl::edge_entry<Edge, Keys...>{&target};
}

/// @brief The graph's nodes, in declaration order.
///
/// Nodes and edges are two lists rather than one because they are two kinds of
/// thing, and every place the graph used to ask an entry which it was is a
/// place it now does not have to: the index layout walks the node list, the
/// error walks the edge list, and neither has to skip over the other.
template <class... Entries>
struct nodes {
  explicit constexpr nodes(Entries... es) : entries{es...} {
  }

  impl::pack<Entries...> entries;
};

template <class... Entries>
nodes(Entries...) -> nodes<Entries...>;

/// @brief The graph's edges, in declaration order.
template <class... Entries>
struct edges {
  explicit constexpr edges(Entries... es) : entries{es...} {
  }

  impl::pack<Entries...> entries;
};

template <class... Entries>
edges(Entries...) -> edges<Entries...>;

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_ENTRIES_HPP
