/// ===============================================================================================
/// @file
///
/// @brief The graph: an algorithm, keyed nodes, the edges over them, the dual
/// index layout, and the fit.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_HPP
#define UZU_OPTIMIZATION_GRAPH_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "uzu/foundation/dual/functional/apply.hpp"
#include "uzu/foundation/dual/functional/summation.hpp"
#include "uzu/foundation/dual/functional/zip.hpp"
#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"
#include "uzu/helpers/keywords.hpp"
#include "uzu/optimization/graph_contracts.hpp"
#include "uzu/optimization/graph_edge.hpp"
#include "uzu/optimization/graph_kernel.hpp"
#include "uzu/optimization/graph_node.hpp"

namespace uzu {
namespace impl {

/// @brief A node in the graph's node list: an estimate, under a key.
template <std::size_t Key, class Node>
struct node_entry {
  static constexpr std::size_t key = Key;
  static constexpr std::size_t dimension = Node::dimension;

  Node *target;
};

/// @brief An edge in the graph's edge list, over the keys it links.
template <class Edge, std::size_t... Keys>
struct link_entry {
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
template <std::size_t... Keys, class Edge>
constexpr auto link(Edge &target) {
  return impl::link_entry<Edge, Keys...>{&target};
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

/// @brief A factor graph over keyed nodes and the edges between them, fitted
/// by the algorithm it is given.
template <class Algorithm, class Nodes, class Edges>
class graph;

/// @brief The graph proper.
///
/// Keys are what make the index layout possible. A node's identity is a
/// compile-time constant, so it gets exactly one block of dual indices no
/// matter how many edges name it, and each edge finds its nodes by looking
/// their keys up in the node list. Nothing has to be matched at run time.
///
/// @tparam Algorithm Builds what turns a gradient into a step.
/// @tparam Ns The node entries, whose order fixes the index layout.
/// @tparam Ls The edges and the keys each links, in any order.
template <class Algorithm, class... Ns, class... Ls>
class graph<Algorithm, nodes<Ns...>, edges<Ls...>> {
  static constexpr std::size_t node_count = sizeof...(Ns);
  static constexpr std::size_t edge_count = sizeof...(Ls);

  template <std::size_t I>
  using node_at = std::tuple_element_t<I, std::tuple<Ns...>>;

  template <std::size_t I>
  using edge_at = std::tuple_element_t<I, std::tuple<Ls...>>;

  static constexpr std::array<std::size_t, node_count> keys{Ns::key...};
  static constexpr std::array<std::size_t, node_count> widths{Ns::dimension...};

  static_assert(impl::distinct_keys(keys), "two nodes are declared under the same key");
  static_assert(impl::edges_resolve<Ls...>(keys), "an edge links a key no node declares");

  /// @brief Which node entry carries @p wanted, or #node_count if none does.
  static constexpr auto node_of(std::size_t wanted) -> std::size_t {
    return impl::find_key(keys, wanted);
  }

 public:
  using value_type = double;

  /// @brief How many dual indices the graph takes up: one per dimension of
  /// each declared node, and nothing more.
  static constexpr std::size_t width = (Ns::dimension + ... + 0);

  explicit constexpr graph(const Algorithm &algorithm, nodes<Ns...> ns, edges<Ls...> ls)
      : algorithm_{algorithm.template build<width>()}, nodes_{ns.entries}, edges_{ls.entries} {
  }

  /// @brief Runs the fit, in place. The step size belongs to the algorithm;
  /// what is said here is only how many passes to take.
  template <class... Args>
  constexpr auto fit(const Args &...args) -> void {
    const auto passes = impl::find_one<impl::iterations_tag>(10, args...);

    for (auto i = decltype(passes){0}; i < passes; ++i) {
      descend();
    }
  }

  /// @brief The total error of the current estimates, robustified.
  constexpr auto error() const {
    return total().value();
  }

  /// @brief The derivative of that error with respect to every graph index,
  /// in the order the nodes were declared. Exposed for diagnostics and for
  /// checking the derivatives against finite differences.
  constexpr auto gradient() const {
    return spread(total(), std::make_index_sequence<width>{});
  }

 private:
  /// @brief What the algorithm builds for a graph this wide.
  ///
  /// An algorithm keeping one value per dual index wants that storage sized by the width, and the
  /// width is not known where the algorithm is written - so the algorithm is a builder, and this
  /// is what it builds, asked for here where the width is settled.
  using algorithm_type = typename Algorithm::template instance<width>;

  /// @brief Where the node under @p wanted starts in the index space, or
  /// #width if no node carries that key.
  static constexpr auto offset_of(std::size_t wanted) -> std::size_t {
    const std::size_t node = node_of(wanted);

    std::size_t at = 0;
    for (std::size_t i = 0; i < node; ++i) {
      at += widths[i];
    }
    return at;
  }

  /// @brief The node declared under @p Key.
  template <std::size_t Key>
  constexpr auto &at() const {
    return *std::get<node_of(Key)>(nodes_).target;
  }

  /// @brief A zero dual carrying every index in the graph.
  ///
  /// Added to the accumulated error so the total is guaranteed to track all
  /// of them. Without it, an edge that happens not to depend on one of its
  /// nodes' dimensions would leave that index out of the result type, and
  /// asking for its derivative would not compile.
  template <std::size_t... Ks>
  static constexpr auto zero(std::index_sequence<Ks...>) {
    return dual::number<value_type, Ks...>{value_type{0}, value_type{0}};
  }

  /// @brief The estimates of one edge's nodes, seeded as duals.
  template <std::size_t I, std::size_t... Js>
  constexpr auto seeds(std::index_sequence<Js...>) const {
    return std::tuple{
        at<edge_at<I>::keys[Js]>().template seed<offset_of(edge_at<I>::keys[Js])>()...};
  }

  /// @brief One edge's contribution: its residual, each dimension
  /// through the kernel with that dimension's sigma, summed.
  ///
  /// The residual is zipped against the sigmas and summed with the library's
  /// own summation, so the shape here is the same one the bezier fit uses -
  /// a fold over paired sequences rather than an index-driven loop.
  template <std::size_t I>
  constexpr auto residual() const {
    const auto *e = std::get<I>(edges_).target;

    const auto raw = dual::apply(
        [&](const auto &...vs) { return e->error(vs...); },
        seeds<I>(std::make_index_sequence<edge_at<I>::arity>{}));

    static_assert(
        std::decay_t<decltype(raw)>::size() == edge_at<I>::target_type::dimension,
        "an edge's error returns a different number of residuals than it declares");

    return dual::summation(dual::zip(raw, e->sigma()), [](const auto &residual, const auto &sigma) {
      return kernel::radial(residual, sigma);
    });
  }

  /// @brief The total error across every edge, as a dual.
  template <std::size_t... Is>
  constexpr auto total(std::index_sequence<Is...>) const {
    return (zero(std::make_index_sequence<width>{}) + ... + residual<Is>());
  }

  constexpr auto total() const {
    return total(std::make_index_sequence<edge_count>{});
  }

  /// @brief Every derivative of the total, in graph index order.
  template <class Total, std::size_t... Ks>
  static constexpr auto spread(const Total &value, std::index_sequence<Ks...>) {
    return std::array<value_type, width>{value.template dvalue<Ks>()...};
  }

  /// @brief Moves one node along its own slice of the total's derivatives.
  ///
  /// The kernel is applied here, not in the algorithm. It reads the node's
  /// sigma, which is the graph's data, and what it does - make a gradient
  /// comparable across dimensions of different scales, and bound it - is a
  /// property of the problem rather than of the rule used to descend it. What
  /// reaches the algorithm is the sign of the derivative and a magnitude below
  /// one, whichever dimension it came from.
  ///
  /// The braced list fixes the order the algorithm is called in: its elements
  /// are evaluated left to right, and the nodes are walked in declaration
  /// order, so a stateful algorithm sees its indices in index order.
  template <std::size_t I, class Total, std::size_t... Ks>
  constexpr auto move(const Total &value, std::index_sequence<Ks...>) -> void {
    auto &self = *std::get<I>(nodes_).target;
    constexpr auto offset = offset_of(node_at<I>::key);

    self.update({algorithm_.template step<offset + Ks>(
        kernel::signed_radial(value.template dvalue<offset + Ks>(), self.sigma()[Ks]))...});
  }

  /// @brief Moves every node once, along the gradient of the total error.
  template <std::size_t... Is>
  constexpr auto descend(std::index_sequence<Is...>) -> void {
    const auto value = total();

    (move<Is>(value, std::make_index_sequence<node_at<Is>::dimension>{}), ...);
  }

  constexpr auto descend() -> void {
    descend(std::make_index_sequence<node_count>{});
  }

  algorithm_type algorithm_;
  std::tuple<Ns...> nodes_;
  std::tuple<Ls...> edges_;
};

template <class Algorithm, class... Ns, class... Ls>
graph(Algorithm, nodes<Ns...>, edges<Ls...>) -> graph<Algorithm, nodes<Ns...>, edges<Ls...>>;

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_HPP
