/// ===============================================================================================
/// @file
///
/// @brief `uzu::node_like`, `uzu::edge_like` and `uzu::algorithm_for`: what passes, and the
/// mistakes each one is there to catch.
///
/// A graph built from a type that fails one does not compile, so the concepts are asserted here
/// directly, both ways round. Every check is a `static_assert`, so this file passing its tests
/// *is* this file compiling.
/// ===============================================================================================

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>

#include "tests/fixtures/simple_graph.hpp"
#include "tests/helpers/compile_time.hpp"
#include "uzu.h"

namespace {

using uzu::test::absolute;
using uzu::test::point2;
using uzu::test::relative;
using estimate = std::array<double, 2>;

// ---------------------------------------------------------------------------------------------
// Nodes.
// ---------------------------------------------------------------------------------------------

static_assert(uzu::node_like<point2>, "the fixture node is one");

/// @brief Derived from the base, but never says how a delta moves it.
struct no_plus : uzu::node<no_plus, estimate, 2> {
  using base = uzu::node<no_plus, estimate, 2>;
  using base::base;
};

/// @brief Says how to move, but names another class as the one derived from the base - the
/// copy-paste slip that, unchecked, has the base call `point2::plus` on a `wrong_self`.
struct wrong_self : uzu::node<point2, estimate, 2> {
  using base = uzu::node<point2, estimate, 2>;
  using base::base;

  constexpr auto plus(const point &) const -> estimation_type {
    return estimation();
  }
};

static_assert(!uzu::node_like<no_plus>, "a node needs a plus");
static_assert(!uzu::node_like<wrong_self>, "and must name itself in its base");
static_assert(!uzu::node_like<absolute>, "an edge is not a node");
static_assert(!uzu::node_like<estimate>, "nor is what a node holds");

// ---------------------------------------------------------------------------------------------
// Edges.
// ---------------------------------------------------------------------------------------------

static_assert(uzu::edge_like<absolute> && uzu::edge_like<relative>, "the fixture edges are");

/// @brief The same slip on the edge side.
struct wrong_edge : uzu::edge<absolute, estimate, 2> {
  using base = uzu::edge<absolute, estimate, 2>;
  using base::base;
};

static_assert(!uzu::edge_like<wrong_edge>, "an edge must name itself in its base");
static_assert(!uzu::edge_like<point2>, "a node is not an edge");

// Whether an edge's error fits the nodes it is linked to depends on the link, so it is checked
// there. These are the inputs a two-dimensional node is seeded as.
using seeded = decltype(std::declval<const point2 &>().seed<0>());

static_assert(uzu::impl::scores<absolute, std::tuple<seeded>>, "one node for a one-node edge");
static_assert(uzu::impl::scores<relative, std::tuple<seeded, seeded>>, "two for a two-node edge");
static_assert(!uzu::impl::scores<relative, std::tuple<seeded>>, "but not one for two");

// ---------------------------------------------------------------------------------------------
// Algorithms.
// ---------------------------------------------------------------------------------------------

static_assert(uzu::algorithm_for<uzu::gradient, 4>, "gradient");
static_assert(uzu::algorithm_for<uzu::momentum, 4>, "momentum");
static_assert(uzu::algorithm_for<uzu::rprop, 4>, "rprop");
static_assert(uzu::algorithm_for<uzu::momentum, 0>, "at any width, an empty graph's too");

/// @brief Names an instance but cannot build one.
struct no_build {
  using value_type = double;
  template <std::size_t Width>
  using instance = no_build;
};

/// @brief Builds an instance whose step takes nothing to step on.
struct no_slope {
  using value_type = double;
  template <std::size_t Width>
  using instance = no_slope;
  template <std::size_t Width>
  constexpr auto build() const -> instance<Width> {
    return *this;
  }
  template <std::size_t Index>
  constexpr auto step() -> value_type {
    return 0.0;
  }
};

static_assert(!uzu::algorithm_for<no_build, 4>, "an algorithm must build its instance");
static_assert(!uzu::algorithm_for<no_slope, 4>, "whose step takes a slope");
static_assert(!uzu::algorithm_for<point2, 4>, "a node is not an algorithm");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("graph_concepts");
}
