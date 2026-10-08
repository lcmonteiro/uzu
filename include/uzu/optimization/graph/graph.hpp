/// ===============================================================================================
/// @file
///
/// @brief The graph: keyed nodes, the edges over them, and the fit that moves the nodes.
///
/// This header is the orchestration and little else. The pieces it puts together each live where
/// they can be read on their own:
///
///     meta::keyed_layout      which block of dual indices each node takes up
///     graph/entries.hpp       how a node or an edge enters the graph
///     graph/contracts.hpp     what the edges must satisfy
///     dual::zero/derivatives  a total that tracks every index, and reading it back out
///     kernel.hpp              robustifying an error, bounding a gradient
///     the Algorithm           turning a bounded gradient into a step
///
/// and what is left is the flow between them:
///
///     error()     -> total_error() -> edge_error() for each edge
///     gradient()  -> dual::derivatives(total_error())
///     fit()       -> descend() -> total_error(), then update_node() for each node
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_GRAPH_HPP
#define UZU_OPTIMIZATION_GRAPH_GRAPH_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "uzu/foundation/dual/derivatives.hpp"
#include "uzu/foundation/dual/functional/apply.hpp"
#include "uzu/foundation/dual/functional/summation.hpp"
#include "uzu/foundation/dual/functional/zip.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"
#include "uzu/foundation/meta/keyed_layout.hpp"
#include "uzu/helpers/keywords.hpp"
#include "uzu/optimization/graph/contracts.hpp"
#include "uzu/optimization/graph/edge.hpp"
#include "uzu/optimization/graph/entries.hpp"
#include "uzu/optimization/graph/node.hpp"
#include "uzu/optimization/kernel.hpp"

namespace uzu {

/// @brief A factor graph over keyed nodes and the edges between them, fitted
/// by the algorithm it is given.
template <class Algorithm, class Nodes, class Edges>
class graph;

/// @brief The graph proper.
///
/// Keys are what make the index layout possible. A node's identity is a
/// compile-time constant, so it gets exactly one block of dual indices no
/// matter how many edges name it, and each edge finds its nodes by looking
/// their keys up in the layout. Nothing has to be matched at run time.
///
/// @tparam Algorithm Builds what turns a gradient into a step.
/// @tparam Ns The node entries, whose order fixes the index layout.
/// @tparam Es The edges and the keys each links, in any order.
template <class Algorithm, class... Ns, class... Es>
class graph<Algorithm, nodes<Ns...>, edges<Es...>> {
  /// @brief One block of dual indices per node, in declaration order.
  using layout = meta::keyed_layout<Ns...>;

  static_assert(layout::distinct_keys, "two nodes are declared under the same key");
  static_assert(impl::edges_resolve<layout, Es...>, "an edge links a key no node declares");

  /// @brief The entry of the edge declared @p I-th: its type and the keys it links.
  template <std::size_t I>
  using edge_entry_at = std::tuple_element_t<I, std::tuple<Es...>>;

 public:
  using value_type = double;

  /// @brief How many dual indices the graph takes up: one per dimension of
  /// each declared node, and nothing more.
  static constexpr std::size_t width = layout::width;

  explicit constexpr graph(const Algorithm &algorithm, nodes<Ns...> ns, edges<Es...> es)
      : algorithm_{algorithm.template build<width>()}, nodes_{ns.entries}, edges_{es.entries} {
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
    return total_error().value();
  }

  /// @brief The derivative of that error with respect to every graph index,
  /// in the order the nodes were declared. Exposed for diagnostics and for
  /// checking the derivatives against finite differences.
  constexpr auto gradient() const {
    return dual::derivatives<0, width>(total_error());
  }

 private:
  /// @brief What the algorithm builds for a graph this wide.
  ///
  /// An algorithm keeping one value per dual index wants that storage sized by the width, and the
  /// width is not known where the algorithm is written - so the algorithm is a builder, and this
  /// is what it builds, asked for here where the width is settled.
  using algorithm_type = typename Algorithm::template instance<width>;

  // ---------------------------------------------------------------------------------------------
  // The error.
  // ---------------------------------------------------------------------------------------------

  /// @brief The total error across every edge, as a dual carrying every graph index.
  ///
  /// It starts from a zero over the whole index space so that the result tracks every index:
  /// an edge that happens not to depend on one of its nodes' dimensions would otherwise leave
  /// that index out of the result's type, and asking for its derivative would not compile.
  constexpr auto total_error() const {
    return total_error(std::make_index_sequence<sizeof...(Es)>{});
  }

  template <std::size_t... Is>
  constexpr auto total_error(std::index_sequence<Is...>) const {
    return (dual::zero<value_type, width>() + ... + edge_error<Is>());
  }

  /// @brief One edge's contribution to the total: its error evaluated on its nodes' seeded
  /// estimates, each residual robustified against that dimension's sigma, and summed.
  ///
  /// The residual is zipped against the sigmas and summed with the library's
  /// own summation, so the shape here is the same one the bezier fit uses -
  /// a fold over paired sequences rather than an index-driven loop.
  template <std::size_t I>
  constexpr auto edge_error() const {
    using entry = edge_entry_at<I>;
    const auto &edge = edge_at<I>();

    const auto residuals = dual::apply(
        [&](const auto &...inputs) { return edge.error(inputs...); },
        edge_inputs<I>(std::make_index_sequence<entry::arity>{}));

    static_assert(
        std::decay_t<decltype(residuals)>::size() == entry::target_type::dimension,
        "an edge's error returns a different number of residuals than it declares");

    return dual::summation(
        dual::zip(residuals, edge.sigma()),
        [](const auto &residual, const auto &sigma) { return kernel::radial(residual, sigma); });
  }

  /// @brief What edge @p I is evaluated on: the estimate of each node it links, seeded as duals
  /// at that node's block in the graph's index space.
  template <std::size_t I, std::size_t... Js>
  constexpr auto edge_inputs(std::index_sequence<Js...>) const {
    using entry = edge_entry_at<I>;
    return std::tuple{
        node_by_key<entry::keys[Js]>().template seed<layout::offset_of(entry::keys[Js])>()...};
  }

  /// @brief The edge declared @p I-th.
  template <std::size_t I>
  constexpr auto edge_at() const -> auto & {
    return *std::get<I>(edges_).target;
  }

  /// @brief The node declared under @p Key.
  template <std::size_t Key>
  constexpr auto node_by_key() const -> auto & {
    return node_at<layout::index_of(Key)>();
  }

  /// @brief The node declared @p I-th.
  template <std::size_t I>
  constexpr auto node_at() const -> auto & {
    return *std::get<I>(nodes_).target;
  }

  // ---------------------------------------------------------------------------------------------
  // The fit.
  // ---------------------------------------------------------------------------------------------

  /// @brief Moves every node once, along the gradient of the total error.
  ///
  /// The error is evaluated once per pass, before any node moves, so every node steps along the
  /// same gradient. Nodes are updated in declaration order - which is index order - so a stateful
  /// algorithm sees its indices in order.
  constexpr auto descend() -> void {
    descend(std::make_index_sequence<layout::size>{});
  }

  template <std::size_t... Is>
  constexpr auto descend(std::index_sequence<Is...>) -> void {
    const auto total = total_error();

    (update_node<Is>(total, std::make_index_sequence<layout::template entry_at<Is>::width>{}), ...);
  }

  /// @brief Steps node @p I along its own slice of the total error's derivatives.
  ///
  /// Each derivative is bounded by the kernel against the node's sigma before the algorithm sees
  /// it. That is applied here, not in the algorithm: sigma is the graph's data, and making a
  /// gradient comparable across dimensions of different scales is a property of the problem
  /// rather than of the rule used to descend it. What reaches the algorithm is the sign of the
  /// derivative and a magnitude below one, whichever dimension it came from.
  ///
  /// The kernel is pure, so the whole slope is normalised before the algorithm is called. The
  /// braced list then fixes the order the algorithm is called in: its elements are evaluated left
  /// to right, so within a node, too, the algorithm sees its indices in order.
  template <std::size_t I, class Total, std::size_t... Ks>
  constexpr auto update_node(const Total &total, std::index_sequence<Ks...>) -> void {
    using entry = typename layout::template entry_at<I>;
    constexpr auto offset = layout::offset_of(entry::key);
    auto &node = node_at<I>();
    const auto slope = dual::derivatives<offset, entry::width>(total);
    const auto slope_normalized = kernel::signed_radial(slope, node.sigma());
    node.update({algorithm_.template step<offset + Ks>(slope_normalized[Ks])...});
  }

  algorithm_type algorithm_;
  std::tuple<Ns...> nodes_;
  std::tuple<Es...> edges_;
};

template <class Algorithm, class... Ns, class... Es>
graph(Algorithm, nodes<Ns...>, edges<Es...>) -> graph<Algorithm, nodes<Ns...>, edges<Es...>>;

}  // namespace uzu

#endif  // UZU_OPTIMIZATION_GRAPH_GRAPH_HPP
