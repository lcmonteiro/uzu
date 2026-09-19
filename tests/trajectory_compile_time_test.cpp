/// ===============================================================================================
/// @file
///
/// @brief The property `optimization_trajectory_test.cpp` checks, at a size where the compiler can
/// check it: a chain of poses, each with a prior and a constraint to its neighbour, recovered from
/// a single initial guess they all start from.
///
/// This is the companion to that test rather than a smaller copy of it. There, the problem is
/// noisy and randomized and the question is statistical -- does the trajectory come back, on
/// average and at worst. Here there is no noise and the question is exact: a consistent set of
/// measurements has one answer, and the fit must find it. Nothing is deferred to a run, so a
/// chain that fails to recover is a compile error naming the property.
///
/// The reference is a zig-zag rather than the sine wave, because the sine needs `std::sin` and
/// none of `<cmath>` can run in a constant expression before C++26. That is the same reason the
/// kernel is built from squarings rather than from `exp` -- see `graph_kernel.hpp`.
///
/// Size is what keeps this buildable. Four poses is eight dual indices and eleven passes of a
/// thousand; the randomized test's twenty-four poses would be forty-eight indices carried through
/// ninety-six residuals, interpreted rather than executed. The two tests together are the trade:
/// this one proves the fit exactly at four poses, that one measures it at twenty-four.
/// ===============================================================================================

#include <array>
#include <cstddef>
#include <utility>

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
using uzu::test::magnitude;

namespace {

constexpr auto kPoses = std::size_t{4};
constexpr auto kSpacing = 4.0;
constexpr auto kHeight = 3.0;

/// @brief A pose in the plane, free to move in both dimensions.
struct pose : uzu::node<pose, std::array<double, 2>, 2> {
  using base = uzu::node<pose, std::array<double, 2>, 2>;
  using base::base;

  template <class Delta>
  constexpr auto plus(const Delta &delta) const -> estimation_type {
    return {estimation()[0] + delta[0], estimation()[1] + delta[1]};
  }
};

/// @brief An absolute prior: where one pose was measured to be.
struct prior : uzu::edge<prior, std::array<double, 2>, 2> {
  using base = uzu::edge<prior, std::array<double, 2>, 2>;
  using base::base;

  template <class B>
  constexpr auto error(const B &b) const {
    return b - measurement();
  }
};

/// @brief A relative constraint: the measured offset from one pose to the next.
struct between : uzu::edge<between, std::array<double, 2>, 2> {
  using base = uzu::edge<between, std::array<double, 2>, 2>;
  using base::base;

  template <class A, class B>
  constexpr auto error(const A &a, const B &b) const {
    return (b - a) - measurement();
  }
};

/// @brief The reference pose at @p index: a zig-zag, so that the y constraints are not all alike.
constexpr auto reference(std::size_t index) -> std::array<double, 2> {
  return {static_cast<double>(index) * kSpacing, index % 2 == 0 ? kHeight : -kHeight};
}

/// @brief What the fit reached, and how far it stayed from the reference.
struct recovery {
  double worst;
  double before;
  double after;
};

/// @tparam Is one index per pose -- the node list, and the prior on each.
/// @tparam Js one index per link in the chain, so one fewer than the poses.
template <std::size_t... Is, std::size_t... Js>
constexpr auto recover(std::index_sequence<Is...>, std::index_sequence<Js...>) -> recovery {
  auto poses = std::array<pose, sizeof...(Is)>{};
  auto priors = std::array<prior, sizeof...(Is)>{};
  auto links = std::array<between, sizeof...(Js)>{};

  // Every pose starts at the origin, so the fit has to separate them as well as place them.
  for (std::size_t i = 0; i < sizeof...(Is); ++i) {
    poses[i] = pose(init = {0.0, 0.0}, sigma = {0.01, 0.01});

    const auto truth = reference(i);
    priors[i] = prior(init = {truth[0], truth[1]}, sigma = {30.0, 30.0});
  }
  for (std::size_t j = 0; j < sizeof...(Js); ++j) {
    const auto from = reference(j);
    const auto to = reference(j + 1);

    links[j] = between(init = {to[0] - from[0], to[1] - from[1]}, sigma = {30.0, 30.0});
  }

  auto graph = uzu::graph{
      gradient{lr = 0.2},
      nodes{key<Is>(poses[Is])...},
      edges{link<Is>(priors[Is])..., link<Js, Js + 1>(links[Js])...}};

  const auto before = graph.error();
  graph.fit(iterations = 1000);

  double worst = 0.0;
  for (std::size_t i = 0; i < sizeof...(Is); ++i) {
    const auto truth = reference(i);
    const auto &got = poses[i].estimation();

    worst = magnitude(got[0] - truth[0]) > worst ? magnitude(got[0] - truth[0]) : worst;
    worst = magnitude(got[1] - truth[1]) > worst ? magnitude(got[1] - truth[1]) : worst;
  }
  return {.worst = worst, .before = before, .after = graph.error()};
}

constexpr auto fitted =
    recover(std::make_index_sequence<kPoses>{}, std::make_index_sequence<kPoses - 1>{});

static_assert(fitted.after < fitted.before, "the fit lowers the error it is minimising");
static_assert(fitted.worst < 1.0, "every pose lands within a unit of the reference");
static_assert(fitted.after < 1e-2, "and the consistent set is very nearly satisfied");

}  // namespace

/// @brief Everything above has already passed by the time this runs; reaching it means the
/// compiler evaluated the fit and every assertion held.
auto main() -> int {
  return uzu::test::passed("trajectory (compile time)");
}
