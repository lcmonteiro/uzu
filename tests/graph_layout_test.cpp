/// ===============================================================================================
/// @file
///
/// @brief What the index layout gives each node, and what reaches it.
///
/// One block per declared node, however many edges name it -- and a node no edge names is laid
/// out all the same, and simply does not move. With identity settled at compile time there is
/// no second block to find and add up, so what could go wrong is not the bookkeeping but the
/// accumulation: both edges naming one node must actually reach the same index.
///
/// Every check here is a `static_assert`, so the fits below are run by the compiler and this file
/// passing its tests *is* this file compiling. Nothing is deferred to a run: each scenario is a
/// `constexpr` function returning what it measured, each result is a `constexpr` variable, and
/// each property is asserted against it by name.
/// ===============================================================================================

#include <array>
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
using uzu::test::close;
using uzu::test::point2;
namespace {

// ---------------------------------------------------------------------------------------------
// A node named by two edges carries one block, holding the sum of what reaches it through each.
//
// This is the property the keys buy. With identity settled at compile time there is no second
// block to find and add up, so the thing that could go wrong is not the bookkeeping but the
// accumulation itself: both edges must actually reach the same index.
// ---------------------------------------------------------------------------------------------

struct shared_report {
  std::size_t width;
  double both;
  double single;
  double moved;
  double expected;
};

constexpr auto measure_shared() -> shared_report {
  auto twice = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});

  auto e1 = absolute(init = {3.0, 3.0}, sigma = {2.0});
  auto e2 = absolute(init = {3.0, 3.0}, sigma = {2.0});

  auto both = uzu::graph{gradient{lr = 0.1}, nodes{key<1>(twice)}, edges{link<1>(e1), link<1>(e2)}};

  auto once = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});
  auto e3 = absolute(init = {3.0, 3.0}, sigma = {2.0});

  auto single = uzu::graph{gradient{lr = 0.1}, nodes{key<1>(once)}, edges{link<1>(e3)}};

  const auto reached = both.gradient()[0];
  const auto alone = single.gradient()[0];

  const auto before = twice.estimation()[0];
  both.fit(iterations = 1);

  return {
      .width = both.width,
      .both = reached,
      .single = alone,
      .moved = twice.estimation()[0] - before,
      .expected = -0.1 * uzu::kernel::signed_radial(2 * alone, 0.5)};
}

constexpr auto shared = measure_shared();

static_assert(shared.width == 2, "two edges on one node still lay out one block");
static_assert(
    close(shared.both, 2 * shared.single, 1e-12), "the block holds the sum of both edges");
static_assert(close(shared.moved, shared.expected, 1e-12), "and it steps once, on that sum");
static_assert(shared.moved > 0.0, "and it steps towards the measurement");

// ---------------------------------------------------------------------------------------------
// A node no edge names is laid out, and simply does not move.
// ---------------------------------------------------------------------------------------------

struct unlinked_report {
  std::size_t width;
  double x;
  double y;
};

constexpr auto measure_unlinked() -> unlinked_report {
  auto used = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});
  auto spare = point2(init = {5.0, 6.0}, sigma = {0.5, 0.5});

  auto e = absolute(init = {3.0, 3.0}, sigma = {2.0});

  auto g = uzu::graph{gradient{lr = 0.1}, nodes{key<1>(used), key<2>(spare)}, edges{link<1>(e)}};

  g.fit(iterations = 5);

  return {.width = g.width, .x = spare.estimation()[0], .y = spare.estimation()[1]};
}

constexpr auto unlinked = measure_unlinked();

static_assert(unlinked.width == 4, "an unlinked node still takes its block");
static_assert(unlinked.x == 5.0, "an unlinked node does not move");
static_assert(unlinked.y == 6.0, "in either dimension");

}  // namespace

/// @brief Everything above has already passed by the time this runs; reaching it means the
/// compiler evaluated every fit and every assertion held.
auto main() -> int {
  return uzu::test::passed("layout");
}
