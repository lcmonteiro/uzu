/// ===============================================================================================
/// @file
///
/// @brief The derivatives, against central finite differences.
///
/// This is the check that covers the whole chain at once: the seeding of one dual index per
/// node dimension, the kernel applied to the residual, both accumulations, and the slicing of
/// the result back out. It is the one that would catch a wrong index offset.
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
// The derivatives, against central finite differences.
//
// This is the check that covers the whole chain at once: the seeding of one dual index per node
// dimension, the kernel applied to the residual, both accumulations, and the slicing of the result
// back out. It is the one that would catch a wrong index offset.
// ---------------------------------------------------------------------------------------------

struct derivative_report {
  std::size_t width;
  double worst;
};

constexpr auto measure_derivatives() -> derivative_report {
  auto n1 = point2(init = {0.3, -0.7}, sigma = {1.0, 1.0});
  auto n2 = point2(init = {1.9, 0.4}, sigma = {1.0, 1.0});

  auto e1 = relative(sigma = {2.0, 3.0});
  auto e2 = absolute(init = {0.0, 0.0}, sigma = {2.0});
  auto e3 = absolute(init = {4.0, 2.0}, sigma = {1.5});

  auto g = uzu::graph{
      gradient{lr = 0.1},
      nodes{key<1>(n1), key<2>(n2)},
      edges{link<1, 2>(e1), link<1>(e2), link<2>(e3)}};

  const auto analytic = g.gradient();

  // Node 1 was declared first, so it holds indices 0 and 1; node 2 holds 2 and 3. Two edges name
  // each of them and neither gets a second block.
  point2 *of[]{&n1, &n2};

  // A central difference divides the noise in the two values by `h`, and the kernel's value
  // carries about `2^kernel::order` ulp of rounding, so the usable step is tied to the order.
  // At 4 that is 16 ulp and 1e-6 sits in the floor, at 5e-10; at 20 it would read noise as
  // slope and this check would fail. Raising the order means raising this too.
  const double h = 1e-6;
  double worst = 0.0;

  for (std::size_t n = 0; n < 2; ++n) {
    for (std::size_t k = 0; k < 2; ++k) {
      const auto held = of[n]->estimation();

      auto up = held;
      up[k] += h;
      of[n]->estimation(up);
      const auto above = g.error();

      auto down = held;
      down[k] -= h;
      of[n]->estimation(down);
      const auto below = g.error();

      of[n]->estimation(held);

      const auto numeric = (above - below) / (2 * h);
      const auto apart = magnitude(analytic[n * 2 + k] - numeric);

      worst = apart > worst ? apart : worst;
    }
  }

  return {.width = analytic.size(), .worst = worst};
}

constexpr auto derivatives = measure_derivatives();

static_assert(derivatives.width == 4, "one block per declared node, however many edges name it");
static_assert(derivatives.worst <= 1e-6, "every derivative matches a central difference");

// ---------------------------------------------------------------------------------------------
// An edge with a single residual.
//
// The residual and the sigmas are zipped before they are summed, and a one-row zip is the case
// class template argument deduction gets wrong: `std::tuple(row)` over a tuple `row` deduces a
// copy of it instead of wrapping it, so the sum would run over the row's elements. This graph
// only compiles if one residual comes through as one row.
// ---------------------------------------------------------------------------------------------

/// @brief The residual `x + y - m`: one number from a two-dimensional node.
struct sum_of_coordinates : uzu::edge<sum_of_coordinates, std::array<double, 1>, 1> {
  using base = uzu::edge<sum_of_coordinates, std::array<double, 1>, 1>;
  using base::base;

  template <class A>
  constexpr auto error(const A &a) const {
    return dual::array{std::get<0>(a) + std::get<1>(a) - measurement()[0]};
  }
};

constexpr auto measure_single_residual() -> derivative_report {
  auto n = point2(init = {0.3, -0.7}, sigma = {1.0, 1.0});
  auto e = sum_of_coordinates(init = {1.0}, sigma = {2.0});

  auto g = uzu::graph{gradient{lr = 0.1}, nodes{key<0>(n)}, edges{link<0>(e)}};
  const auto analytic = g.gradient();

  const double h = 1e-6;
  double worst = 0.0;
  for (std::size_t k = 0; k < 2; ++k) {
    const auto held = n.estimation();
    auto up = held;
    up[k] += h;
    n.estimation(up);
    const auto above = g.error();
    auto down = held;
    down[k] -= h;
    n.estimation(down);
    const auto below = g.error();
    n.estimation(held);

    const auto apart = magnitude(analytic[k] - (above - below) / (2 * h));
    worst = apart > worst ? apart : worst;
  }
  return {.width = analytic.size(), .worst = worst};
}

constexpr auto single_residual = measure_single_residual();

static_assert(single_residual.width == 2, "one block for the one node");
static_assert(single_residual.worst <= 1e-6, "a one-residual edge differentiates like any other");

}  // namespace

/// @brief Everything above has already passed by the time this runs; reaching it means the
/// compiler evaluated every fit and every assertion held.
auto main() -> int {
  return uzu::test::passed("derivatives");
}
