/// ===============================================================================================
/// @file
///
/// @brief The keyword arguments: broadcasting, and that order does not matter.
///
/// `sigma = {2.0}` stands for a whole row of alike sigmas and `sigma = {2.0, 3.0}` sets them
/// apart, and a node built `init` first means the same as one built `sigma` first.
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

using uzu::init;
using uzu::sigma;
using uzu::test::absolute;
using uzu::test::point2;
using uzu::test::relative;
namespace {

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
  return uzu::test::passed("keywords");
}
