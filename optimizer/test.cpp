/// @file test.cpp
/// @brief Checks the parts that could be wrong without the fit looking wrong: the derivatives,
/// and the accounting for a node named by more than one edge.
///
/// Every check here is a `static_assert`, so the fits below are run by the compiler and this file
/// passing its tests *is* this file compiling. Nothing is deferred to a run: each scenario is a
/// `constexpr` function returning what it measured, each result is a `constexpr` variable, and
/// each property is asserted against it by name.
///
/// That costs something and buys something. It costs compile time - the fits are interpreted, not
/// executed - and it needs a raised `-fconstexpr-ops-limit`. It buys a test that cannot be built
/// and then not run, cannot pass on one machine and fail on another, and reports a failed property
/// by its own message at the line that states it.
///
///   g++ -std=c++20 -O2 -fconstexpr-ops-limit=100000000 -I. -I../dual test.cpp -o test

#include <array>
#include <cstddef>
#include <cstdio>

#include "uzu/graph.hpp"

using uzu::beta;
using uzu::edges;
using uzu::gradient;
using uzu::init;
using uzu::iterations;
using uzu::key;
using uzu::link;
using uzu::lr;
using uzu::momentum;
using uzu::nodes;
using uzu::sigma;

namespace {

/// @brief `std::fabs` is not usable at compile time, and this is all of it that is needed.
constexpr auto magnitude(double x) -> double {
  return x < 0.0 ? -x : x;
}

constexpr auto close(double a, double b, double tolerance) -> bool {
  return magnitude(a - b) <= tolerance;
}

struct point2 : uzu::node<point2, std::array<double, 2>, 2> {
  using base = uzu::node<point2, std::array<double, 2>, 2>;
  using base::base;

  template <class Delta>
  constexpr auto plus(const Delta &delta) const -> estimation_type {
    auto out = estimation();
    for (std::size_t i = 0; i < out.size(); ++i) {
      out[i] += delta[i];
    }
    return out;
  }
};

struct relative : uzu::edge<relative, std::array<double, 0>, 2> {
  using base = uzu::edge<relative, std::array<double, 0>, 2>;
  using base::base;

  template <class A, class B>
  constexpr auto error(const A &a, const B &b) const {
    return b - a;
  }
};

struct absolute : uzu::edge<absolute, std::array<double, 2>, 2> {
  using base = uzu::edge<absolute, std::array<double, 2>, 2>;
  using base::base;

  template <class B>
  constexpr auto error(const B &b) const {
    return b - measurement();
  }
};

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
  auto e2 = absolute(sigma = {2.0});
  auto e3 = absolute(sigma = {1.5});

  e2.measurement({0.0, 0.0});
  e3.measurement({4.0, 2.0});

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

  auto e1 = absolute(sigma = {2.0});
  auto e2 = absolute(sigma = {2.0});
  e1.measurement({3.0, 3.0});
  e2.measurement({3.0, 3.0});

  auto both = uzu::graph{gradient{lr = 0.1}, nodes{key<1>(twice)}, edges{link<1>(e1), link<1>(e2)}};

  auto once = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});
  auto e3 = absolute(sigma = {2.0});
  e3.measurement({3.0, 3.0});

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

  auto e = absolute(sigma = {2.0});
  e.measurement({3.0, 3.0});

  auto g = uzu::graph{gradient{lr = 0.1}, nodes{key<1>(used), key<2>(spare)}, edges{link<1>(e)}};

  g.fit(iterations = 5);

  return {.width = g.width, .x = spare.estimation()[0], .y = spare.estimation()[1]};
}

constexpr auto unlinked = measure_unlinked();

static_assert(unlinked.width == 4, "an unlinked node still takes its block");
static_assert(unlinked.x == 5.0, "an unlinked node does not move");
static_assert(unlinked.y == 6.0, "in either dimension");

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
  auto e2 = absolute(sigma = {2.0});
  auto e3 = absolute(sigma = {2.0});

  e2.measurement({0.0, 0.0});
  e3.measurement({4.0, 2.0});

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

// ---------------------------------------------------------------------------------------------
// Momentum.
// ---------------------------------------------------------------------------------------------

/// @brief One point pulled at by one measurement, built fresh so two algorithms can be given the
/// very same problem.
template <class Algorithm>
constexpr auto pulled(point2 &at, absolute &by, Algorithm algorithm) {
  return uzu::graph{algorithm, nodes{key<1>(at)}, edges{link<1>(by)}};
}

/// @brief With no velocity carried over, momentum is plain descent.
///
/// `beta = 0` leaves `v = g`, so the two rules are the same arithmetic in a different order, and
/// the estimates have to agree exactly rather than closely. An error here would mean the velocity
/// is being applied when it should be inert - the one way momentum can be wrong that still
/// converges.
struct decay_report {
  double plain_x;
  double heavy_x;
  double plain_y;
  double heavy_y;
};

constexpr auto measure_no_decay() -> decay_report {
  auto a = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});
  auto b = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});

  auto e1 = absolute(sigma = {2.0});
  auto e2 = absolute(sigma = {2.0});
  e1.measurement({3.0, -2.0});
  e2.measurement({3.0, -2.0});

  auto plain = pulled(a, e1, gradient{lr = 0.1});
  auto heavy = pulled(b, e2, momentum{lr = 0.1, beta = 0.0});

  plain.fit(iterations = 50);
  heavy.fit(iterations = 50);

  return {
      .plain_x = a.estimation()[0],
      .heavy_x = b.estimation()[0],
      .plain_y = a.estimation()[1],
      .heavy_y = b.estimation()[1]};
}

constexpr auto no_decay = measure_no_decay();

static_assert(no_decay.plain_x == no_decay.heavy_x, "beta = 0 steps identically");
static_assert(no_decay.plain_y == no_decay.heavy_y, "in either dimension");

/// @brief The velocity is the bounded gradient, accumulated and bled.
///
/// Checked against the rule written out by hand over two passes: the first step is `-lr * g1`, and
/// the second `-lr * (beta * g1 + g2)`. Reading the gradients back between passes is what makes
/// this a check of the accumulation rather than of the fit.
struct velocity_report {
  double first;
  double first_expected;
  double second;
  double second_expected;
};

constexpr auto measure_velocity() -> velocity_report {
  const double rate = 0.1;
  const double decay = 0.7;

  auto n = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});

  auto e = absolute(sigma = {2.0});
  e.measurement({3.0, -2.0});

  auto g = pulled(n, e, momentum{lr = rate, beta = decay});

  const auto start = n.estimation()[0];
  const auto g1 = uzu::kernel::signed_radial(g.gradient()[0], 0.5);
  g.fit(iterations = 1);
  const auto first = n.estimation()[0];

  const auto g2 = uzu::kernel::signed_radial(g.gradient()[0], 0.5);
  g.fit(iterations = 1);

  return {
      .first = first - start,
      .first_expected = -rate * g1,
      .second = n.estimation()[0] - first,
      .second_expected = -rate * (decay * g1 + g2)};
}

constexpr auto velocity = measure_velocity();

static_assert(
    close(velocity.first, velocity.first_expected, 1e-12), "the first step is the plain one");
static_assert(
    close(velocity.second, velocity.second_expected, 1e-12),
    "the second carries beta of the first");

/// @brief One builder, handed to two graphs, gives two velocities.
///
/// What the caller writes carries the hyper-parameters and nothing else; the state appears when a
/// graph builds an instance at its own width. So the same builder can be reused, and two graphs
/// built from it cannot tread on each other. Running one of them hard and then starting the other
/// must leave the second taking its plain first step, exactly as if the first had never run.
struct builder_report {
  double stepped;
  double expected;
};

constexpr auto measure_builder() -> builder_report {
  const auto builder = momentum{lr = 0.1, beta = 0.9};

  auto a = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});
  auto e1 = absolute(sigma = {2.0});
  e1.measurement({3.0, -2.0});

  auto first = pulled(a, e1, builder);
  first.fit(iterations = 100);

  auto b = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});
  auto e2 = absolute(sigma = {2.0});
  e2.measurement({3.0, -2.0});

  auto second = pulled(b, e2, builder);

  const auto start = b.estimation()[0];
  const auto g1 = uzu::kernel::signed_radial(second.gradient()[0], 0.5);
  second.fit(iterations = 1);

  return {.stepped = b.estimation()[0] - start, .expected = -0.1 * g1};
}

constexpr auto builder = measure_builder();

static_assert(close(builder.stepped, builder.expected, 1e-12), "a second graph starts from rest");

/// @brief Along a slope that keeps pointing the same way, momentum is faster.
///
/// The velocity settles at `g / (1 - beta)`, so the step grows to several times the plain one
/// while the gradient is consistent. At the same `lr`, momentum must reach a given error in
/// strictly fewer passes; if it ever needed more, the velocity would be fighting the gradient
/// rather than compounding it.
template <class Algorithm>
constexpr auto passes_to(Algorithm algorithm, double target, int limit) -> int {
  auto n = point2(init = {1.0, 1.0}, sigma = {0.5, 0.5});
  auto e = absolute(sigma = {2.0});
  e.measurement({3.0, -2.0});

  auto g = pulled(n, e, algorithm);

  for (int i = 1; i <= limit; ++i) {
    g.fit(iterations = 1);
    if (g.error() < target) {
      return i;
    }
  }
  return 0;
}

constexpr auto plain_passes = passes_to(gradient{lr = 0.02}, 0.02, 2000);
constexpr auto heavy_passes = passes_to(momentum{lr = 0.02, beta = 0.9}, 0.02, 2000);

static_assert(plain_passes > 0, "plain descent reaches the threshold at all");
static_assert(heavy_passes > 0, "and so does momentum");
static_assert(heavy_passes < plain_passes, "momentum reaches it in fewer passes");

// ---------------------------------------------------------------------------------------------
// The keywords.
// ---------------------------------------------------------------------------------------------

constexpr auto one_sigma = absolute(sigma = {2.5});
constexpr auto two_sigmas = relative(sigma = {2.0, 3.0});

static_assert(one_sigma.sigma()[0] == 2.5, "sigma broadcasts to dimension 0");
static_assert(one_sigma.sigma()[1] == 2.5, "sigma broadcasts to dimension 1");
static_assert(two_sigmas.sigma()[0] == 2.0, "sigma per dimension, first");
static_assert(two_sigmas.sigma()[1] == 3.0, "sigma per dimension, second");

constexpr auto in_order = point2(init = {1.0, 2.0}, sigma = {3.0, 4.0});
constexpr auto reversed = point2(sigma = {3.0, 4.0}, init = {1.0, 2.0});

static_assert(in_order.estimation()[0] == reversed.estimation()[0], "init found either way");
static_assert(in_order.sigma()[1] == reversed.sigma()[1], "sigma found either way");

}  // namespace

/// @brief Everything above has already passed by the time this runs; reaching it means the
/// compiler evaluated every fit and every assertion held.
auto main() -> int {
  std::printf("all passed, at compile time\n");
  std::printf("  (passes to error < 0.02: descent %d, momentum %d)\n", plain_passes, heavy_passes);
  return 0;
}
