/// ===============================================================================================
/// @file
///
/// @brief The smallest graph worth testing: a 2D point, a relative constraint between two of
/// them, and an absolute one against a measurement.
///
/// Every test in this folder is built from these three. A node supplies `plus`, an edge supplies
/// `error`, and neither knows the graph exists -- so the fixture is the whole of what a user of
/// the library writes, and testing against it tests the same surface they meet.
/// ===============================================================================================
#ifndef UZU_TESTS_FIXTURES_SIMPLE_GRAPH_HPP
#define UZU_TESTS_FIXTURES_SIMPLE_GRAPH_HPP

#include <array>
#include <cstddef>

#include "uzu.h"

namespace uzu::test {

/// @brief A point in the plane, free to move in both dimensions.
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

/// @brief Two points, scored by how far apart they are. Carries no measurement of its own.
struct relative : uzu::edge<relative, std::array<double, 0>, 2> {
  using base = uzu::edge<relative, std::array<double, 0>, 2>;
  using base::base;

  template <class A, class B>
  constexpr auto error(const A &a, const B &b) const {
    return b - a;
  }
};

/// @brief One point, scored against where it was measured to be.
struct absolute : uzu::edge<absolute, std::array<double, 2>, 2> {
  using base = uzu::edge<absolute, std::array<double, 2>, 2>;
  using base::base;

  template <class B>
  constexpr auto error(const B &b) const {
    return b - measurement();
  }
};

/// @brief One point pulled at by one measurement, built fresh so that two algorithms can be
/// handed the very same problem.
template <class Algorithm>
constexpr auto pulled(point2 &at, absolute &by, Algorithm algorithm) {
  return uzu::graph{algorithm, uzu::nodes{uzu::key<1>(at)}, uzu::edges{uzu::link<1>(by)}};
}

}  // namespace uzu::test

#endif  // UZU_TESTS_FIXTURES_SIMPLE_GRAPH_HPP
