/// ===============================================================================================
/// @file
///
/// @brief `dual::zero` and `dual::derivatives`: a zero over a range of indices, and reading a
/// range of derivatives back out.
///
/// Every check here is a `static_assert`, so this file passing its tests *is* this file compiling.
/// ===============================================================================================

#include <cstddef>

#include "tests/helpers/compile_time.hpp"
#include "uzu/foundation/dual/derivatives.hpp"
#include "uzu/foundation/dual/number.hpp"
#include "uzu/foundation/dual/operations/plus.hpp"

namespace {

// ---------------------------------------------------------------------------------------------
// A zero over [0, N).
// ---------------------------------------------------------------------------------------------

constexpr auto none = dual::zero<double, 4>();

static_assert(none.size == 4, "one derivative per index in the range");
static_assert(none.value() == 0.0, "the value is zero");
static_assert(none.dvalue<0>() == 0.0 && none.dvalue<3>() == 0.0, "every derivative is zero");

// Starting a sum from it widens the result to every index in the range, so an index no term
// touched still reads - as zero - instead of failing to compile.
constexpr auto sparse = none + dual::number<double, 1>{5.0, 2.0};

static_assert(sparse.value() == 5.0, "adding the zero leaves the value alone");
static_assert(sparse.dvalue<1>() == 2.0, "and the derivatives a term carried");
static_assert(sparse.dvalue<3>() == 0.0, "and reads zero where no term reached");

// ---------------------------------------------------------------------------------------------
// Reading derivatives out.
// ---------------------------------------------------------------------------------------------

constexpr auto all = dual::derivatives<0, 4>(sparse);

static_assert(all.size() == 4, "one per index read");
static_assert(all[0] == 0.0 && all[1] == 2.0 && all[3] == 0.0, "in index order");

constexpr auto slice = dual::derivatives<1, 2>(sparse);

static_assert(slice.size() == 2, "a slice is as long as asked");
static_assert(slice[0] == 2.0 && slice[1] == 0.0, "and starts where asked");

static_assert(dual::derivatives<2, 0>(sparse).size() == 0, "an empty slice is allowed");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("dual_derivatives");
}
