/// ===============================================================================================
/// @file
///
/// @brief Randomized trajectory optimization: build a noisy problem along a sine-wave reference
/// trajectory -- random initial poses, noisy absolute priors, and noisy relative loop closures --
/// and check the fit recovers the reference.
///
/// This is vortex's `optimization_trajectory_test` asked of a gradient-descent fit. Two scenarios
/// share one problem generator, as they do there: a small, fast one, and a larger one that
/// stresses the same pipeline at twice the poses and twice the closures.
///
/// It is the one test in this suite that runs rather than compiles. The rest are `static_assert`s
/// over fits the compiler evaluates; here the compiler is already doing the expensive part -- a
/// graph of 24 poses is 48 dual indices, and every one of the 96 residuals carries all of them --
/// and a constant-evaluated fit of a thousand passes on top of that is not a trade worth making.
/// `trajectory_compile_time_test.cpp` holds the property this one checks, at a size where it is.
///
/// The bounds are read off what the fit actually reaches rather than tuned until they pass, which
/// is the brittleness vortex's own scenario comments warn against. The small one meets vortex's
/// own 3.0, and both hold the mean under 1.5 -- and it is the mean that says the trajectory as a
/// whole is recovered, while the worst case is what catches a single pose left behind. See the
/// note on #kLargeMaxError for why only the worst case grows with the pose count.
/// ===============================================================================================

#include <cstddef>
#include <cstdio>
#include <utility>

#include "tests/fixtures/trajectory_graph.hpp"
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
using uzu::test::accuracy;
using uzu::test::between;
using uzu::test::closures;
using uzu::test::pose;
using uzu::test::prior;
using uzu::test::reference;
using uzu::test::scenario;
using uzu::test::stream;

namespace {

/// @brief The small scenario: 12 poses, 48 loop closures, a 24-index system.
///
/// Where vortex puts 100 poses and 1000 closures, this puts 12 and 48. The difference is not the
/// solver but the graph: vortex's is built at run time, so its size costs a loop iteration, while
/// uzu's is a type, so every pose is a template instantiation and every closure another. That
/// cost is quadratic in the pose count here -- 8 poses compile in 6 s, 24 in 30, 48 in 63 -- so
/// 100 poses is not a unit test, it is a build.
constexpr auto kSmall = scenario{
    .nodes = 12,
    .links_per_node = 4,
    .amplitude = 10.0,
    .width = 80.0,
    .guess_min = -10.0,
    .guess_max = 100.0,
    .noise = 0.15,
    .node_sigma = 0.01,
    .edge_sigma = 30.0,
    .rate = 0.2,
    .seed = 1,
    .passes = 2000,
};

/// @brief What the small scenario must reach. The worst case is the bound vortex holds its own
/// small scenario to; measured, this fit reaches 2.65 and a mean of 1.06.
constexpr auto kSmallMaxError = 3.0;
constexpr auto kSmallMeanError = 1.5;

/// @brief The same problem at twice the size: 24 poses, 96 closures, a 48-index system.
constexpr auto kLarge = scenario{
    .nodes = 24,
    .links_per_node = 4,
    .amplitude = 10.0,
    .width = 80.0,
    .guess_min = -10.0,
    .guess_max = 100.0,
    .noise = 0.15,
    .node_sigma = 0.01,
    .edge_sigma = 30.0,
    .rate = 0.2,
    .seed = 1,
    .passes = 2000,
};

/// @brief What the large scenario must reach: the same mean, a looser worst case.
///
/// Measured, this fit reaches a worst case of 3.36 and a mean of 1.26. The mean does not move
/// with the pose count and the worst case does, and the reason is the noise rather than the fit:
/// each prior is drawn with up to 15% multiplicative error, so doubling the poses doubles the
/// chances of drawing one bad enough to strand its pose, and there is no linear solve here to
/// average it back out. Raising the closure density barely touches it -- measured at 24 poses,
/// six closures each gives 3.39 and eight gives 3.12 -- and running longer makes it worse, 3.71
/// at four times the passes, because the extra passes fit the noisy prior more exactly rather
/// than less. That is the saturating kernel behaving as documented: this fit is robust, not
/// least-squares.
constexpr auto kLargeMaxError = 4.0;
constexpr auto kLargeMeanError = 1.5;

/// ===============================================================================================
/// @brief The two sigmas, since they are the whole of what had to be tuned.
///
/// They are not two names for one quantity. `edge_sigma` is the residual's: it says where a
/// measurement stops being ordinary, and past `sqrt(2N) * sigma` -- 5.66 sigma at the default
/// kernel order -- a residual contributes nothing and pulls not at all. Initial guesses here are
/// up to 90 units from the truth, so at vortex's noise-scale sigma every pose would start in the
/// kernel's dead zone and the fit would not move at all. 30.0 puts the rejection point past the
/// worst initial residual, so every pose is still being pulled on pass one.
///
/// `node_sigma` is the gradient's, and it works the other way. The kernel bounds what reaches the
/// algorithm into [0, 1], and a gradient small against its sigma comes out proportionally
/// smaller, so a *large* node sigma means small steps. 0.01 keeps the step near `lr` while the
/// pose is far from where it belongs.
///
/// Both were found by sweeping, and both are the problem's rather than the library's -- which is
/// the substantive difference from vortex, where Levenberg-Marquardt picks its own step and the
/// only thing to state is the iteration budget. Here the two sigmas and the rate are three
/// numbers the problem has to supply, and getting them wrong does not converge slowly, it does
/// not converge at all: at vortex's sigma scale this same problem moves the error from 29.6 to
/// 28.9 over two thousand passes and leaves every pose where it started.
/// ===============================================================================================

/// @brief Builds, fits and measures the problem @p setup describes.
///
/// @tparam Is one index per pose -- the node list, and the prior on each.
/// @tparam Js one index per loop closure.
template <const scenario &Setup, std::size_t... Is, std::size_t... Js>
auto recover(std::index_sequence<Is...>, std::index_sequence<Js...>) -> accuracy {
  constexpr auto pairs = closures<sizeof...(Is), Setup.links_per_node>();

  auto poses = std::array<pose, sizeof...(Is)>{};
  auto priors = std::array<prior, sizeof...(Is)>{};
  auto loops = std::array<between, sizeof...(Js)>{};

  // The measurements are drawn at run time; only the pairing above had to be settled earlier.
  auto random = stream{Setup.seed};
  const auto noisy = [&random](double value) {
    return value * random.uniform(1.0 - Setup.noise, 1.0 + Setup.noise);
  };

  for (std::size_t i = 0; i < sizeof...(Is); ++i) {
    poses[i] = pose(sigma = {Setup.node_sigma, Setup.node_sigma});
    poses[i].estimation({
        random.uniform(Setup.guess_min, Setup.guess_max),
        random.uniform(Setup.guess_min, Setup.guess_max),
    });

    const auto truth = reference(i, Setup);
    priors[i] = prior(
        init = {noisy(truth[0]), noisy(truth[1])}, sigma = {Setup.edge_sigma, Setup.edge_sigma});
  }

  for (std::size_t j = 0; j < sizeof...(Js); ++j) {
    const auto from = reference(pairs[j].first, Setup);
    const auto to = reference(pairs[j].second, Setup);

    loops[j] = between(
        init = {noisy(to[0] - from[0]), noisy(to[1] - from[1])},
        sigma = {Setup.edge_sigma, Setup.edge_sigma});
  }

  auto graph = uzu::graph{
      gradient{lr = Setup.rate},
      nodes{key<Is>(poses[Is])...},
      edges{link<Is>(priors[Is])..., link<pairs[Js].first, pairs[Js].second>(loops[Js])...}};

  static_assert(graph.width == 2 * sizeof...(Is), "one block per pose, two indices each");

  const auto before = graph.error();
  graph.fit(iterations = Setup.passes);
  const auto after = graph.error();

  std::printf(
      "  %zu poses, %zu closures, %zu indices: error %g -> %g\n",  //
      sizeof...(Is),
      sizeof...(Js),
      graph.width,
      before,
      after);

  return uzu::test::measure(poses, Setup);
}

/// @brief Runs one scenario and reports whether it met its bounds.
template <const scenario &Setup>
auto check(const char *name, double max_error, double mean_error) -> bool {
  const auto error = recover<Setup>(
      std::make_index_sequence<Setup.nodes>{},
      std::make_index_sequence<Setup.nodes * Setup.links_per_node>{});

  std::printf(
      "  %s: worst %g at pose %zu, mean %g\n", name, error.worst, error.worst_at, error.mean);

  auto held = true;
  if (!(error.worst < max_error)) {
    std::printf("  FAILED: worst %g is not below %g\n", error.worst, max_error);
    held = false;
  }
  if (!(error.mean < mean_error)) {
    std::printf("  FAILED: mean %g is not below %g\n", error.mean, mean_error);
    held = false;
  }
  return held;
}

}  // namespace

auto main() -> int {
  auto held = true;
  held = check<kSmall>("small", kSmallMaxError, kSmallMeanError) && held;
  held = check<kLarge>("large", kLargeMaxError, kLargeMeanError) && held;

  if (!held) {
    std::printf("trajectory: FAILED\n");
    return 1;
  }
  std::printf("trajectory: the reference is recovered at both scales\n");
  return 0;
}
