/// ===============================================================================================
/// @file
///
/// @brief A graph with more edges than a fold expression may have arguments.
///
/// Clang refuses to instantiate a fold expression over more than 256 arguments, because the fold
/// expands into an expression nested that deep. The graph used to visit its edges, check them and
/// sum its layout with folds, so a graph of 257 edges did not compile on clang at all - whatever
/// the edges were. It now does all three through flat initializer lists. This file is the graph
/// that used to be refused: one node, and 300 measurements of it.
///
/// Every edge has the same type and links the same key, so this costs little to compile; what it
/// exercises is the count, not the variety.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include <array>
#include <cstddef>
#include <utility>

#include "tests/fixtures/simple_graph.hpp"
#include "tests/helpers/compile_time.hpp"
#include "uzu.h"

using uzu::edges;
using uzu::init;
using uzu::iterations;
using uzu::key;
using uzu::link;
using uzu::lr;
using uzu::nodes;
using uzu::rprop;
using uzu::sigma;
using uzu::test::absolute;
using uzu::test::close;
using uzu::test::point2;

namespace {

/// @brief More than clang's limit of 256 fold arguments.
constexpr std::size_t kEdges = 300;

struct many_report {
  std::size_t width;
  double error;
  double expected_error;
  double gradient_x;
  double x;
  double y;
};

/// @brief Half the measurements put the node at (1, 2), half at (3, 4), and it starts at the
/// origin. Checked are the three places that used to fold over the edges: the total error (every
/// edge visited), its gradient, and one pass of the fit (every node visited after them).
template <std::size_t... Is>
constexpr auto measure(std::index_sequence<Is...>) -> many_report {
  auto n = point2(init = {0.0, 0.0}, sigma = {0.01, 0.01});

  auto measured = std::array<absolute, sizeof...(Is)>{};
  for (std::size_t i = 0; i < measured.size(); ++i) {
    measured[i] = i % 2 == 0 ? absolute(init = {1.0, 2.0}, sigma = {4.0})
                             : absolute(init = {3.0, 4.0}, sigma = {4.0});
  }

  auto g = uzu::graph{rprop{lr = 0.1}, nodes{key<0>(n)}, edges{link<0>(measured[Is])...}};

  const auto half = static_cast<double>(sizeof...(Is) / 2);
  const auto expected =
      half * (uzu::kernel::radial(1.0, 4.0) + uzu::kernel::radial(2.0, 4.0)) +
      half * (uzu::kernel::radial(3.0, 4.0) + uzu::kernel::radial(4.0, 4.0));

  const auto error = g.error();
  const auto slope = g.gradient()[0];
  g.fit(iterations = 1);

  return {
      .width = g.width,
      .error = error,
      .expected_error = expected,
      .gradient_x = slope,
      .x = n.estimation()[0],
      .y = n.estimation()[1],
  };
}

constexpr auto many = measure(std::make_index_sequence<kEdges>{});

static_assert(many.width == 2, "300 edges on one node still take up one block");
static_assert(close(many.error, many.expected_error, 1e-9), "every one of the 300 edges is counted");
static_assert(many.gradient_x < 0.0, "and pulls the node towards the measurements");
static_assert(close(many.x, 0.1, 1e-12) && close(many.y, 0.1, 1e-12), "one pass of the fit moves it");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("many edges");
}
