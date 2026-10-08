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
#include <tuple>

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

  std::tuple<Entries...> entries;
};

template <class... Entries>
nodes(Entries...) -> nodes<Entries...>;

/// @brief The graph's edges, in declaration order.
template <class... Entries>
struct edges {
  explicit constexpr edges(Entries... es) : entries{es...} {
  }

  std::tuple<Entries...> entries;
};

template <class... Entries>
edges(Entries...) -> edges<Entries...>;

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_ENTRIES_HPP
