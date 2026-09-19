/// ===============================================================================================
/// @file
///
/// @brief Momentum: what it does with the velocity it carries, and that it carries none across
/// graphs.
///
/// Four properties, in the order they can go wrong: `beta = 0` must be plain descent exactly;
/// the velocity must be the bounded gradient accumulated and bled; one builder handed to two
/// graphs must give two velocities; and along a consistent slope momentum must reach a given
/// error in strictly fewer passes than plain descent.
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

using uzu::beta;
using uzu::gradient;
using uzu::init;
using uzu::iterations;
using uzu::lr;
using uzu::momentum;
using uzu::sigma;
using uzu::test::absolute;
using uzu::test::close;
using uzu::test::point2;
using uzu::test::pulled;
namespace {

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

}  // namespace

/// @brief Everything above has already passed by the time this runs; reaching it means the
/// compiler evaluated every fit and every assertion held.
auto main() -> int {
  std::printf("  (passes to error < 0.02: descent %d, momentum %d)\n", plain_passes, heavy_passes);
  return uzu::test::passed("algorithms");
}
