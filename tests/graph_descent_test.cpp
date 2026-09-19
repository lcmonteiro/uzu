/// ===============================================================================================
/// @file
///
/// @brief The fit must not increase the error it is minimising, and an exact symmetry must survive
/// it.
///
/// Reflecting both nodes through the midpoint of the two measurements leaves the objective
/// unchanged, so a fit started on that set must stay on it. That is an exact invariant rather
/// than a limit, so it catches an asymmetry in the gradient split that a converged-position
/// check would not: a run that ends in the right place can still have taken a crooked path.
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
using uzu::test::magnitude;
using uzu::test::point2;
using uzu::test::relative;
namespace {

// ---------------------------------------------------------------------------------------------
// The fit must not increase the error it is minimising, and an exact symmetry must survive it.
//
// Reflecting both nodes through the midpoint of the two measurements leaves the objective
// unchanged, so a fit started on that set must stay on it. That is an exact invariant rather than
// a limit, so it catches an asymmetry in the gradient split that a converged-position check would
// not: a run that ends in the right place can still have taken a crooked path.
// ---------------------------------------------------------------------------------------------

struct descent_report {
  bool never_rose;
  double worst_x;
  double worst_y;
};

constexpr auto measure_descent() -> descent_report {
  auto n1 = point2(init = {1.5, 0.5}, sigma = {0.4, 0.4});
  auto n2 = point2(init = {2.5, 1.5}, sigma = {0.4, 0.4});

  auto e1 = relative(sigma = {2.0, 3.0});
  auto e2 = absolute(init = {0.0, 0.0}, sigma = {2.0});
  auto e3 = absolute(init = {4.0, 2.0}, sigma = {2.0});

  auto g = uzu::graph{
      gradient{lr = 0.05},
      nodes{key<1>(n1), key<2>(n2)},
      edges{link<1, 2>(e1), link<1>(e2), link<2>(e3)}};

  bool never_rose = true;
  double worst_x = 0.0;
  double worst_y = 0.0;

  auto previous = g.error();
  for (int i = 0; i < 200; ++i) {
    g.fit(iterations = 1);

    const auto current = g.error();
    never_rose = never_rose && current <= previous + 1e-12;
    previous = current;

    const auto x = magnitude(n1.estimation()[0] + n2.estimation()[0] - 4.0);
    const auto y = magnitude(n1.estimation()[1] + n2.estimation()[1] - 2.0);

    worst_x = x > worst_x ? x : worst_x;
    worst_y = y > worst_y ? y : worst_y;
  }

  return {.never_rose = never_rose, .worst_x = worst_x, .worst_y = worst_y};
}

constexpr auto descent = measure_descent();

static_assert(descent.never_rose, "each pass lowers the error");
static_assert(descent.worst_x <= 1e-12, "the reflection is preserved in x");
static_assert(descent.worst_y <= 1e-12, "the reflection is preserved in y");

}  // namespace

/// @brief Everything above has already passed by the time this runs; reaching it means the
/// compiler evaluated every fit and every assertion held.
auto main() -> int {
  return uzu::test::passed("descent");
}
