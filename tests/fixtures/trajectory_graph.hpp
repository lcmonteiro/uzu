/// ===============================================================================================
/// @file
///
/// @brief A noisy trajectory-recovery problem, the shape vortex's
/// `optimization_trajectory_test` uses: poses along a reference curve, a noisy absolute prior on
/// each, and noisy relative constraints closing loops between them.
///
/// The problem is the same; expressing it is not. vortex builds its graph at run time --
/// `graph.build<PositionNode>(key)` in a loop -- while a uzu graph is a type: every node carries
/// its key as a template argument and every edge names the keys it links the same way. So the
/// node list and the edge list are produced by pack expansion over an index sequence, and the
/// loop-closure pairs have to be compile-time constants, which is what #closures is for.
///
/// That is the whole of the difference. Once the pack is expanded the graph is built, fitted and
/// measured exactly as vortex's is.
/// ===============================================================================================
#ifndef UZU_TESTS_FIXTURES_TRAJECTORY_GRAPH_HPP
#define UZU_TESTS_FIXTURES_TRAJECTORY_GRAPH_HPP

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <random>
#include <utility>

#include "uzu.h"

namespace uzu::test {

/// @brief Everything that defines one randomized trajectory problem.
struct scenario {
  std::size_t nodes;           //< poses along the reference trajectory
  std::size_t links_per_node;  //< loop closures attempted per pose
  double amplitude;            //< peak height of the reference sine wave
  double width;                //< length of the reference trajectory
  double guess_min;            //< lower corner of the box initial guesses are drawn from
  double guess_max;            //< upper corner of that box
  double noise;                //< multiplicative measurement noise, +/- this fraction
  double node_sigma;           //< the scale the gradient is normalised against
  double edge_sigma;           //< where a residual stops being ordinary
  double rate;                 //< the algorithm's step size
  std::size_t seed;            //< fixes the problem: same seed, same problem, every run
  std::size_t passes;          //< how many passes the fit is given
};

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

/// @brief A loop closure: the measured offset from one pose to another.
struct between : uzu::edge<between, std::array<double, 2>, 2> {
  using base = uzu::edge<between, std::array<double, 2>, 2>;
  using base::base;

  template <class A, class B>
  constexpr auto error(const A &a, const B &b) const {
    return (b - a) - measurement();
  }
};

/// @brief A reproducible source of uniform draws.
///
/// The draws are scaled out of `mt19937`'s raw output rather than taken from
/// `std::uniform_real_distribution`, which is not specified to produce the same sequence across
/// standard libraries. The engine is, so this makes the problem the same problem everywhere -- and
/// the checks below then hold to the same numbers on every platform rather than to whatever
/// bound is loose enough to survive a different one.
class stream {
 public:
  explicit stream(std::size_t seed) : engine_{static_cast<std::mt19937::result_type>(seed)} {
  }

  auto uniform(double low, double high) -> double {
    constexpr auto span = static_cast<double>(std::mt19937::max()) + 1.0;
    return low + (high - low) * (static_cast<double>(engine_()) / span);
  }

 private:
  std::mt19937 engine_;
};

/// @brief The reference point at @p index: a sine wave of @p setup's amplitude over its width.
constexpr auto reference(std::size_t index, const scenario &setup) -> std::array<double, 2> {
  const auto x = static_cast<double>(index) * (setup.width / static_cast<double>(setup.nodes));
  return {x, setup.amplitude * std::sin(2 * std::numbers::pi * (x / setup.width))};
}

/// @brief The loop-closure pairs, as compile-time constants.
///
/// `link<A, B>` takes its keys as template arguments, so the pairs cannot come from the run-time
/// generator the measurements do. They come from a constant-evaluated linear congruential
/// sequence instead: the same arbitrary-but-fixed pairing, settled by the time the graph's type
/// exists. Every pose gets @p Links closures, each to a different pose.
template <std::size_t Nodes, std::size_t Links>
constexpr auto closures() -> std::array<std::pair<std::size_t, std::size_t>, Nodes * Links> {
  auto pairs = std::array<std::pair<std::size_t, std::size_t>, Nodes * Links>{};

  auto state = std::size_t{1};
  const auto next = [&state] {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state >> 33;  // the high bits; the low ones of an LCG cycle far too short
  };

  for (std::size_t i = 0; i < Nodes * Links; ++i) {
    const auto from = i / Links;
    auto to = from;
    while (to == from) {
      to = next() % Nodes;
    }
    pairs[i] = {from, to};
  }
  return pairs;
}

/// @brief Worst and mean distance between the fitted poses and the reference trajectory.
struct accuracy {
  double worst{0.0};        //< largest per-axis deviation of any pose
  double mean{0.0};         //< mean euclidean distance over all poses
  std::size_t worst_at{0};  //< index of the pose holding the worst deviation
};

/// @brief Measures @p poses against the trajectory @p setup describes.
template <class Poses>
auto measure(const Poses &poses, const scenario &setup) -> accuracy {
  auto result = accuracy{};
  for (std::size_t idx = 0; idx < std::size(poses); ++idx) {
    const auto expected = reference(idx, setup);
    const auto &got = poses[idx].estimation();

    const auto deviation =
        std::max(std::fabs(got[0] - expected[0]), std::fabs(got[1] - expected[1]));
    if (deviation > result.worst) {
      result.worst = deviation;
      result.worst_at = idx;
    }
    result.mean += std::hypot(got[0] - expected[0], got[1] - expected[1]);
  }
  result.mean /= static_cast<double>(std::size(poses));
  return result;
}

}  // namespace uzu::test

#endif  // UZU_TESTS_FIXTURES_TRAJECTORY_GRAPH_HPP
