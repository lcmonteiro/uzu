/// ===============================================================================================
/// @file
///
/// @brief A zero over a dense range of derivative indices, and the derivatives of a number over
/// one.
///
/// Both answer to the same situation: a computation whose dual indices are `[0, N)` - one per
/// unknown - and whose result is wanted back as a plain array, or a slice of one. Local to this
/// repository, not part of the upstream snapshot; see docs/dual-storage.md.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_DERIVATIVES_HPP
#define UZU_FOUNDATION_DUAL_DERIVATIVES_HPP

#include <array>
#include <cstddef>
#include <utility>

#include "uzu/foundation/dual/indices.hpp"
#include "uzu/foundation/dual/number.hpp"

namespace dual {

/// @brief A number whose value and every derivative are zero, carrying indices `[0, Count)`.
///
/// A sum's type carries the union of its terms' indices, so a sum that starts from this one
/// carries every index in the range whether or not a term depends on it - and a derivative asked
/// for at an index nothing touched reads zero instead of failing to compile.
/// @tparam T The scalar type.
/// @tparam Count How many indices, starting at 0.
template <class T, std::size_t Count>
constexpr auto zero() {
  using type = typename indices_sequence_t<0, Count>::template type<number, T>;
  return type{T{0}, T{0}};
}

/// @brief Internal details for reading derivatives out.
namespace details {
template <std::size_t Begin, class T, std::size_t... Dn, std::size_t... Ks>
constexpr auto derivatives(const number<T, Dn...> &x, std::index_sequence<Ks...>) {
  return std::array<T, sizeof...(Ks)>{x.template dvalue<Begin + Ks>()...};
}
}  // namespace details

/// @brief The derivatives of @p x at indices `[Begin, Begin + Count)`, in index order.
///
/// Every index in the range must be one @p x carries; one it does not is a compile error, the
/// same as asking `dvalue` for it directly.
/// @tparam Begin The first index.
/// @tparam Count How many indices.
/// @param x The number to read.
/// @return The derivatives, as plain scalars.
template <std::size_t Begin, std::size_t Count, class T, std::size_t... Dn>
constexpr auto derivatives(const number<T, Dn...> &x) -> std::array<T, Count> {
  return details::derivatives<Begin>(x, std::make_index_sequence<Count>{});
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_DERIVATIVES_HPP
