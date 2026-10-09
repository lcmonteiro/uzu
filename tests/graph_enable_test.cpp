/// ===============================================================================================
/// @file
///
/// @brief Enabling and disabling nodes and edges.
///
/// A disabled edge is left out: the graph's error and gradient are exactly those of the same graph
/// declared without it. A disabled node is held: the fit leaves it where it is while the nodes
/// around it still move, and edges keep reading it as a constant. Both switch back on.
///
/// Every check here is a `static_assert`, so the fits below are run by the compiler and this file
/// passing its tests *is* this file compiling.
/// ===============================================================================================

#include <cstddef>

#include "tests/fixtures/simple_graph.hpp"
#include "tests/helpers/compile_time.hpp"
#include "uzu.h"

using uzu::edges;
using uzu::gradient;
using uzu::init;
using uzu::iterations;
using uzu::key;
using uzu::link;
using uzu::lr;
using uzu::nodes;
using uzu::sigma;
using uzu::test::absolute;
using uzu::test::point2;
using uzu::test::relative;

namespace {

// ---------------------------------------------------------------------------------------------
// A disabled edge is as if it were never declared.
// ---------------------------------------------------------------------------------------------

struct edge_report {
  bool enabled_by_default;
  bool same_error;
  bool same_gradient;
  bool back_on;
};

constexpr auto measure_edge() -> edge_report {
  auto p = point2(init = {1.0, 1.0}, sigma = {0.5});
  auto near = absolute(init = {2.0, 0.0}, sigma = {2.0});
  auto far = absolute(init = {-3.0, 4.0}, sigma = {1.0});

  auto both = uzu::graph{gradient{lr = 0.1}, nodes{key<1>(p)}, edges{link<1>(near), link<1>(far)}};
  const auto full = both.error();
  const bool enabled_by_default = far.enabled();

  auto q = point2(init = {1.0, 1.0}, sigma = {0.5});
  auto alone = absolute(init = {2.0, 0.0}, sigma = {2.0});
  auto one = uzu::graph{gradient{lr = 0.1}, nodes{key<1>(q)}, edges{link<1>(alone)}};

  far.disable();
  const bool same_error = both.error() == one.error();
  const bool same_gradient = both.gradient() == one.gradient();

  far.enable();
  const bool back_on = both.error() == full;

  return {enabled_by_default, same_error, same_gradient, back_on};
}

constexpr auto edge = measure_edge();

static_assert(edge.enabled_by_default, "edges start enabled");
static_assert(edge.same_error, "a disabled edge adds nothing to the error");
static_assert(edge.same_gradient, "nor to the gradient");
static_assert(edge.back_on, "and counts again once enabled");

// ---------------------------------------------------------------------------------------------
// With every edge disabled the error is zero, and the fit moves nothing.
// ---------------------------------------------------------------------------------------------

struct silent_report {
  double error;
  double gradient;
  double x;
  double y;
};

constexpr auto measure_silent() -> silent_report {
  auto p = point2(init = {1.0, 1.0}, sigma = {0.5});
  auto pull = absolute(init = {3.0, 3.0}, sigma = {1.0});
  auto g = uzu::graph{gradient{lr = 0.1}, nodes{key<1>(p)}, edges{link<1>(pull)}};

  pull.disable();
  g.fit(iterations = 20);

  return {g.error(), g.gradient()[0], p.estimation()[0], p.estimation()[1]};
}

constexpr auto silent = measure_silent();

static_assert(silent.error == 0.0, "no enabled edge, no error");
static_assert(silent.gradient == 0.0, "and no gradient");
static_assert(silent.x == 1.0 && silent.y == 1.0, "so the fit leaves the node where it was");

// ---------------------------------------------------------------------------------------------
// A disabled node is held: its neighbour still moves towards it, and it does not move at all.
// ---------------------------------------------------------------------------------------------

struct node_report {
  bool enabled_by_default;
  double held_x;
  double held_y;
  double before;
  double after;
  double released_x;
};

constexpr auto measure_node() -> node_report {
  auto anchor = point2(init = {0.0, 0.0}, sigma = {0.5});
  auto free = point2(init = {2.0, 0.0}, sigma = {0.5});
  auto between = relative(sigma = {1.0});
  auto pull = absolute(init = {5.0, 5.0}, sigma = {4.0});

  auto g = uzu::graph{
      gradient{lr = 0.05},
      nodes{key<1>(anchor), key<2>(free)},
      edges{link<1, 2>(between), link<1>(pull)}};

  const bool enabled_by_default = anchor.enabled();
  anchor.disable();

  const auto before = free.estimation()[0];
  g.fit(iterations = 50);
  const auto after = free.estimation()[0];
  const auto held = anchor.estimation();

  anchor.enable();
  g.fit(iterations = 50);

  return {enabled_by_default, held[0], held[1], before, after, anchor.estimation()[0]};
}

constexpr auto node = measure_node();

static_assert(node.enabled_by_default, "nodes start enabled");
static_assert(node.held_x == 0.0 && node.held_y == 0.0, "a disabled node is not moved");
static_assert(node.after < node.before, "while its neighbour still moves towards it");
static_assert(node.released_x != 0.0, "and once enabled it moves again");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("graph_enable");
}
