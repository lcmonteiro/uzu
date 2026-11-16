/// ===============================================================================================
/// @file
///
/// @brief `dual::product` and `dual::transform`: the shape of what they return, and that it folds.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include <array>
#include <cstddef>
#include <tuple>

#include "tests/helpers/compile_time.hpp"
#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/functional/product.hpp"
#include "uzu/foundation/dual/functional/summation.hpp"
#include "uzu/foundation/dual/functional/transform.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"

namespace {

using std::get;

// ---------------------------------------------------------------------------------------------
// The product of plain containers.
// ---------------------------------------------------------------------------------------------

constexpr std::array xs{1.0, 2.0};
constexpr std::array ys{10.0, 20.0, 30.0};

constexpr auto pairs = dual::product(xs, ys);

static_assert(std::tuple_size_v<decltype(pairs)> == 6, "one row per combination");
static_assert(get<0>(get<0>(pairs)) == 1.0 && get<1>(get<0>(pairs)) == 10.0, "starts at (x0, y0)");
static_assert(get<0>(get<1>(pairs)) == 1.0 && get<1>(get<1>(pairs)) == 20.0, "last varies fastest");
static_assert(
    get<0>(get<3>(pairs)) == 2.0 && get<1>(get<3>(pairs)) == 10.0, "then the first moves");
static_assert(get<0>(get<5>(pairs)) == 2.0 && get<1>(get<5>(pairs)) == 30.0, "ends at (x1, y2)");

static_assert(&get<0>(get<0>(pairs)) == &xs[0], "the rows refer into the containers");

// It has the shape zip returns, so summation folds over it: (1 + 2) * (10 + 20 + 30).
static_assert(
    dual::summation(pairs, [](const auto &x, const auto &y) { return x * y; }) == 180.0,
    "summation folds over the rows");

constexpr std::array one{7.0};
constexpr auto single = dual::product(one);

static_assert(std::tuple_size_v<decltype(single)> == 1, "a single row stays a row");
static_assert(get<0>(get<0>(single)) == 7.0, "holding the one element");

constexpr auto triple = dual::product(xs, ys, one);

static_assert(std::tuple_size_v<decltype(triple)> == 6, "a size-one factor adds no rows");
static_assert(std::tuple_size_v<std::tuple_element_t<0, decltype(triple)>> == 3, "but a column");

// ---------------------------------------------------------------------------------------------
// The product of dual arrays, folded into a dual.
// ---------------------------------------------------------------------------------------------

constexpr auto a = dual::make_array<0>(1.0, 2.0);
constexpr auto b = dual::make_array<2>(3.0);

constexpr auto folded =
    dual::summation(dual::product(a, b), [](const auto &x, const auto &y) { return x * y; });

static_assert(folded.value() == 9.0, "(1 + 2) * 3");
static_assert(folded.dvalue<0>() == 3.0 && folded.dvalue<1>() == 3.0, "d/da_i is b");
static_assert(folded.dvalue<2>() == 3.0, "d/db is a0 + a1");

// ---------------------------------------------------------------------------------------------
// transform.
// ---------------------------------------------------------------------------------------------

constexpr auto doubled = dual::transform(a, [](const auto &x) { return x * 2.0; });

static_assert(doubled.size() == 2, "one element per element");
static_assert(get<0>(doubled).value() == 2.0 && get<1>(doubled).value() == 4.0, "f applied");
static_assert(get<1>(doubled).dvalue<1>() == 2.0, "the derivatives carried through f");

constexpr auto squares = dual::transform(xs, [](double x) { return x * x; });

static_assert(squares[0] == 1.0 && squares[1] == 4.0, "and over a std::array");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("dual_functional");
}
