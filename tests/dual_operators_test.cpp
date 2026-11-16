/// ===============================================================================================
/// @file
///
/// @brief What the arithmetic operators accept: a number with a plain scalar of any arithmetic
/// type, and two numbers of the same scalar type.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// The operators are constrained rather than failing in their bodies, so what they reject can be
/// asserted too: a `requires` expression is false for a call that would not compile.
/// ===============================================================================================

#include <type_traits>

#include "tests/helpers/compile_time.hpp"
#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/number.hpp"
#include "uzu/foundation/dual/operations/divides.hpp"
#include "uzu/foundation/dual/operations/minus.hpp"
#include "uzu/foundation/dual/operations/multiplies.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"

namespace {

using x_type = dual::number<double, 0>;
using y_type = dual::number<double, 1>;
using f_type = dual::number<float, 0>;

constexpr x_type x{3.0};

// ---------------------------------------------------------------------------------------------
// A scalar of any arithmetic type is converted to the number's own.
// ---------------------------------------------------------------------------------------------

static_assert((x * 2).value() == 6.0 && (x * 2).dvalue<0>() == 2.0, "an int, on the right");
static_assert((2 - x).value() == -1.0 && (2 - x).dvalue<0>() == -1.0, "an int, on the left");
static_assert((x / 2.0f).value() == 1.5 && (x / 2.0f).dvalue<0>() == 0.5, "a float");
static_assert((x + 1u).value() == 4.0, "an unsigned");

// The result keeps the number's scalar type, whatever the scalar was.
static_assert(std::is_same_v<decltype(x * 2), x_type>, "a double number stays double");
static_assert(std::is_same_v<decltype(f_type{1.0f} * 2.5), f_type>, "a float number stays float");

// Broadcasting over an array goes through the same conversion.
constexpr auto scaled = dual::make_array<0>(1.0, 2.0) * 3;

static_assert(get<1>(scaled).value() == 6.0 && get<1>(scaled).dvalue<1>() == 3.0, "array * int");

// ---------------------------------------------------------------------------------------------
// What is rejected, at the operator rather than inside it.
// ---------------------------------------------------------------------------------------------

template <class A, class B>
constexpr bool addable = requires(const A &a, const B &b) { a + b; };

static_assert(addable<x_type, y_type>, "two numbers of the same scalar type");
static_assert(!addable<x_type, f_type>, "but not of different ones: which would the sum be?");
static_assert(!addable<x_type, bool>, "nor a bool, which is a mistake more often than a 1");
static_assert(!addable<bool, x_type>, "on either side");
static_assert(
    !dual::number_operands<double, int>, "and two plain scalars are left to the language");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("dual_operators");
}
