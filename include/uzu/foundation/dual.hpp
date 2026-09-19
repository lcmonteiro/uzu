/// ===============================================================================================
/// @file
///
/// @brief uzu.foundation.dual umbrella header
///
/// Forward-mode automatic differentiation by dual numbers, vendored from library-dual. Include
/// this to get `dual::number`, `dual::array`, every arithmetic and math operation over them, and
/// the functional layer that folds across an array.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_HPP
#define UZU_FOUNDATION_DUAL_HPP

#include "uzu/foundation/dual/array.hpp"       // IWYU pragma: export
#include "uzu/foundation/dual/functional.hpp"  // IWYU pragma: export
#include "uzu/foundation/dual/number.hpp"      // IWYU pragma: export
#include "uzu/foundation/dual/operations.hpp"  // IWYU pragma: export

#endif  // UZU_FOUNDATION_DUAL_HPP
