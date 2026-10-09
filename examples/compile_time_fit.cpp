/// ===============================================================================================
/// @file
///
/// @brief A whole graph optimization, run by the compiler.
///
/// Four corners of a square, each measured only *relative* to the next one, plus one measurement
/// saying where a single corner sits. No corner is told its own position; the square has to come
/// out of the four sides agreeing with each other, and the anchor then places it.
///
///     c3 ______ c2          sides:  c1 - c0 = ( 2,  0)
///       |      |                    c2 - c1 = ( 0,  2)
///       |      |                    c3 - c2 = (-2,  0)
///       |______|                    c0 - c3 = ( 0, -2)
///     c0        c1          anchor: c0      = ( 1,  1)
///
/// Every corner starts at the origin, on top of every other, so the fit has to separate them as
/// well as place them. The answer is a 2x2 square with its lower-left corner at (1, 1) -- which
/// is worth checking by eye, because the point of this file is not the answer.
///
/// The point is the one line that matters, at the top of `main`:
///
///     constexpr auto fitted = solve();
///
/// `constexpr` there is not decoration. It says the initializer must be a *constant expression*,
/// so the compiler has to run the whole fit -- build the graph, seed the dual numbers, evaluate
/// two hundred passes of gradient descent with momentum -- and fail to compile if it cannot. The
/// `static_assert`s that follow it then check the answer with the program still being compiled.
/// The program exists at all only because every one of them held.
///
/// Nothing is left for the program to do at run time, and it does nothing: `main` returns 0. The
/// optimizer, the graph and even the answer exist only in the compiler, which checked the answer
/// and then had no reason to keep any of it.
/// ===============================================================================================

#include <array>
#include <type_traits>

#include "uzu.h"

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

/// @brief A corner of the square, free to move in the plane.
struct corner : uzu::node<corner, std::array<double, 2>, 2> {
  using base = uzu::node<corner, std::array<double, 2>, 2>;
  using base::base;

  /// @brief How a delta is applied to this estimate. For a point in the plane it is addition, and
  /// it is the only thing an estimate on a manifold would have to change.
  template <class Delta>
  constexpr auto plus(const Delta &delta) const -> estimation_type {
    return {estimation()[0] + delta[0], estimation()[1] + delta[1]};
  }
};

/// @brief One side: how far the next corner should be from this one.
///
/// This function is the whole of what the library is told. Evaluated with `double` it is the
/// residual; evaluated with a dual number it carries its own exact derivative with respect to
/// every coordinate that flowed in. There is no Jacobian written anywhere in this file.
struct side : uzu::edge<side, std::array<double, 2>, 2> {
  using base = uzu::edge<side, std::array<double, 2>, 2>;
  using base::base;

  template <class A, class B>
  constexpr auto error(const A &a, const B &b) const {
    return (b - a) - measurement();
  }
};

/// @brief The anchor: where one corner was measured to be.
struct anchor : uzu::edge<anchor, std::array<double, 2>, 2> {
  using base = uzu::edge<anchor, std::array<double, 2>, 2>;
  using base::base;

  template <class A>
  constexpr auto error(const A &a) const {
    return a - measurement();
  }
};

/// @brief The fitted square, and the error it settled at.
struct square {
  std::array<std::array<double, 2>, 4> corners;
  double error;
};

/// @brief Builds the problem, fits it, and hands back where the corners ended up.
///
/// An ordinary `constexpr` function. Nothing in it is written for the compiler's benefit: the same
/// code, called at run time, does the same thing at run time.
constexpr auto solve() -> square {
  // Every corner starts at the origin. Sigma here is the scale the gradient is normalised
  // against, not a measurement error -- small, so a step stays near the rate while a corner is
  // still far from where it belongs.
  auto c0 = corner(init = {0.0, 0.0}, sigma = {0.005, 0.005});
  auto c1 = corner(init = {0.0, 0.0}, sigma = {0.005, 0.005});
  auto c2 = corner(init = {0.0, 0.0}, sigma = {0.005, 0.005});
  auto c3 = corner(init = {0.0, 0.0}, sigma = {0.005, 0.005});

  // An edge is a measurement and the sigmas it is weighed with, and `init` gives the first the
  // way it gives a corner its estimate. Sigma on an edge is the measurement's: where a residual
  // stops being ordinary. The corners start up to three units from where they belong, so this
  // has to be wide enough that they are still being pulled on the first pass.
  auto bottom = side(init = {2.0, 0.0}, sigma = {4.0, 4.0});
  auto right = side(init = {0.0, 2.0}, sigma = {4.0, 4.0});
  auto top = side(init = {-2.0, 0.0}, sigma = {4.0, 4.0});
  auto closing = side(init = {0.0, -2.0}, sigma = {4.0, 4.0});
  auto at = anchor(init = {1.0, 1.0}, sigma = {4.0, 4.0});

  // Three things in a fixed order: how to step, what to estimate, and what constrains it. The
  // keys are compile-time constants, so each corner gets exactly one block of dual indices no
  // matter how many sides name it, and `link` finds its corners without anything being matched
  // at run time.
  auto graph = uzu::graph{
      momentum{lr = 0.05, beta = 0.9},
      nodes{key<0>(c0), key<1>(c1), key<2>(c2), key<3>(c3)},
      edges{
          link<0, 1>(bottom),
          link<1, 2>(right),
          link<2, 3>(top),
          link<3, 0>(closing),
          link<0>(at)}};

  graph.fit(iterations = 200);

  return {{c0.estimation(), c1.estimation(), c2.estimation(), c3.estimation()}, graph.error()};
}

}  // namespace

auto main() -> int {
  // The fit, run by the compiler. `constexpr` here says the initializer must be a constant
  // expression: if `solve()` could not be constant-evaluated -- if anything on its path reached
  // for `<cmath>`, allocated, or ran past the compiler's evaluation budget -- this line would not
  // compile. It does, so by the time `main` starts, the square is already solved.
  constexpr auto fitted = solve();

  // `std::fabs` cannot run in a constant expression, and this is all of it that is needed.
  constexpr auto close = [](double a, double b) {
    const auto apart = a - b < 0.0 ? b - a : a - b;
    return apart < 0.01;
  };

  // Whether a corner landed at (x, y).
  constexpr auto at = [close](const auto &corner, double x, double y) {
    return close(corner[0], x) && close(corner[1], y);
  };

  // The checks, made while the program is still being compiled. A failure here is a compile
  // error naming the property, at the line that states it -- there is no run in which it could
  // pass.
  static_assert(at(fitted.corners[0], 1.0, 1.0), "the anchored corner sits where it was measured");
  static_assert(at(fitted.corners[1], 3.0, 1.0), "two units along x from it");
  static_assert(at(fitted.corners[2], 3.0, 3.0), "and two up, so the sides are square");
  static_assert(at(fitted.corners[3], 1.0, 3.0), "and the fourth closes the loop");
  static_assert(fitted.error < 1e-6, "the four sides and the anchor are all satisfied at once");

  // The answer is a constant expression, so it can go where only a constant can. A
  // `static_assert` already proves that, but this states it in the form nobody can argue with: a
  // template argument. The side length is the distance between two corners the *optimizer*
  // placed, rounded, and the type system accepts it as a number known at compile time.
  using side_length = std::
      integral_constant<int, static_cast<int>(fitted.corners[1][0] - fitted.corners[0][0] + 0.5)>;

  static_assert(side_length::value == 2, "the square the fit found is two units on a side");

  // Every check above ran while this file was compiled, and the program exists only because they
  // all held. There is nothing left for it to do.
  return 0;
}
