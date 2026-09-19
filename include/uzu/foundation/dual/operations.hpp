/// ===============================================================================================
/// @file
///
/// @brief uzu.foundation.dual.operations umbrella header
///
/// Every operation defined over a dual number. Each one is usable in a constant expression except
/// `exp`, which bottoms out in `std::exp`; see that header for why it stays the exception.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_HPP

#include "uzu/foundation/dual/operations/cos.hpp"         // IWYU pragma: export
#include "uzu/foundation/dual/operations/divides.hpp"     // IWYU pragma: export
#include "uzu/foundation/dual/operations/exp.hpp"         // IWYU pragma: export
#include "uzu/foundation/dual/operations/log.hpp"         // IWYU pragma: export
#include "uzu/foundation/dual/operations/minus.hpp"       // IWYU pragma: export
#include "uzu/foundation/dual/operations/multiplies.hpp"  // IWYU pragma: export
#include "uzu/foundation/dual/operations/negate.hpp"      // IWYU pragma: export
#include "uzu/foundation/dual/operations/plus.hpp"        // IWYU pragma: export
#include "uzu/foundation/dual/operations/pow.hpp"         // IWYU pragma: export
#include "uzu/foundation/dual/operations/sin.hpp"         // IWYU pragma: export
#include "uzu/foundation/dual/operations/sqrt.hpp"        // IWYU pragma: export

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_HPP
