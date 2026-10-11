/// ===============================================================================================
/// @file
///
/// @brief Accumulating a number's derivatives into a dense array, one slot per index.
///
/// A dual number carries only the indices that flowed into it, which keeps each operation as
/// cheap as the computation is local. Many such numbers summed into one dual would widen it to
/// every index, and make every addition pay for all of them. Accumulating into a dense array
/// instead costs one addition per index the number actually carries. Local to this repository, not
/// part of the upstream snapshot; see docs/dual-snapshot.md.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_DERIVATIVES_HPP
#define UZU_FOUNDATION_DUAL_DERIVATIVES_HPP

#include <array>
#include <cstddef>

#include "uzu/foundation/dual/number.hpp"

namespace dual {

/// @brief Adds each derivative of @p x into @p into, at the slot of its own index.
///
/// Slots for indices @p x does not carry are left alone, so accumulating several numbers in turn
/// gives each slot the sum of the derivatives that reached it, and zero where none did.
/// @tparam T The scalar type.
/// @tparam Dn The indices @p x carries; each must be a slot of @p into.
/// @tparam N How many slots @p into has.
/// @param x The number whose derivatives are added.
/// @param into The dense array they are added into.
template <class T, std::size_t... Dn, std::size_t N>
constexpr auto accumulate(const number<T, Dn...> &x, std::array<T, N> &into) -> void {
  static_assert(((Dn < N) && ...), "a derivative index lies outside the array it is added into");
  ((into[Dn] += x.template dvalue<Dn>()), ...);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_DERIVATIVES_HPP
