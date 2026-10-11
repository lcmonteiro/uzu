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

namespace {

// ---------------------------------------------------------------------------------------------
// A zero over [0, N).
// ---------------------------------------------------------------------------------------------

constexpr auto none = dual::zero<double, 4>();

static_assert(none.size == 4, "one derivative per index in the range");
static_assert(none.value() == 0.0, "the value is zero");
static_assert(none.dvalue<0>() == 0.0 && none.dvalue<3>() == 0.0, "every derivative is zero");

// Summing into it: each term adds its value, and each derivative into the slot of its own index.
// Overlapping terms add up; an index no term touched still reads - as zero.
constexpr auto summed() -> dual::dense<double, 4> {
  auto total = dual::zero<double, 4>();
  total += dual::number<double, 1>{5.0, 2.0};
  total += dual::number<double, 1, 2>{1.0, 0.5};
  return total;
}

constexpr auto sparse = summed();

static_assert(sparse.value() == 6.0, "the values add up");
static_assert(sparse.dvalue<1>() == 2.5, "derivatives at a shared index add up");
static_assert(sparse.dvalue<2>() == 0.5, "a derivative only one term carried is that term's");
static_assert(sparse.dvalue<3>() == 0.0, "and an index no term reached reads zero");

// ---------------------------------------------------------------------------------------------
// Reading derivatives out.
// ---------------------------------------------------------------------------------------------

constexpr auto all = dual::derivatives<0, 4>(sparse);

static_assert(all.size() == 4, "one per index read");
static_assert(all[0] == 0.0 && all[1] == 2.5 && all[3] == 0.0, "in index order");

constexpr auto slice = dual::derivatives<1, 2>(sparse);

static_assert(slice.size() == 2, "a slice is as long as asked");
static_assert(slice[0] == 2.5 && slice[1] == 0.5, "and starts where asked");

// A sparse number reads the same way, over the indices it carries.
constexpr auto pair = dual::derivatives<1, 2>(dual::number<double, 1, 2>{1.0, 0.5});

static_assert(pair[0] == 0.5 && pair[1] == 0.5, "a sparse number's own indices read out alike");

static_assert(dual::derivatives<2, 0>(sparse).size() == 0, "an empty slice is allowed");

}  // namespace

/// @brief Everything above has already passed by the time this runs.
auto main() -> int {
  return uzu::test::passed("dual_derivatives");
}
