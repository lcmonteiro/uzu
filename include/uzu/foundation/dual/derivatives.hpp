/// ===============================================================================================
/// @file
///
/// @brief A dense zero over a range of derivative indices, sums into it, and the derivatives of a
/// number over a range.
///
/// All three answer to the same situation: a computation whose dual indices are `[0, N)` - one
/// per unknown - built as a sum of many terms that each carry only a few of them, and whose
/// result is wanted back as a plain array, or a slice of one. Local to this repository, not part
/// of the upstream snapshot; see docs/dual-storage.md.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_DERIVATIVES_HPP
#define UZU_FOUNDATION_DUAL_DERIVATIVES_HPP

#include <array>
#include <cstddef>
#include <utility>

#include "uzu/foundation/dual/number.hpp"

namespace dual {

/// @brief A number carrying every index in `[0, N)`, its derivatives held densely.
///
/// A `number` carries only the indices that flowed into it, which keeps each operation as cheap
/// as the computation is local - but a sum of many local terms, kept as a `number`, widens to the
/// union of all their indices, and then every further term pays for every index. This is the
/// other shape for that sum: one slot per index from the start, and a term added in one step per
/// index *it* carries.
///
/// Deliberately small. It is a sum to add into and read out of, not a second number type with an
/// algebra of its own.
/// @tparam T The scalar type.
/// @tparam N How many indices, starting at 0.
template <class T, std::size_t N>
class dense {
 public:
  using value_type = T;

  static constexpr std::size_t size = N;

  /// @brief The value.
  constexpr auto value() const -> const T & {
    return value_;
  }

  /// @brief The derivative at index @p D.
  template <std::size_t D>
  constexpr auto dvalue() const -> const T & {
    static_assert(D < N, "index is not tracked by this number");
    return dvalues_[D];
  }

  /// @brief Adds a number in: its value, and each derivative into the slot of its own index.
  ///
  /// Slots for indices @p x does not carry are left alone, so each slot ends up holding the sum
  /// of the derivatives that reached it, and zero where none did.
  template <std::size_t... Dn>
  constexpr auto operator+=(const number<T, Dn...> &x) -> dense & {
    static_assert(((Dn < N) && ...), "a derivative index lies outside the range of this number");
    value_ += x.value();
    ((dvalues_[Dn] += x.template dvalue<Dn>()), ...);
    return *this;
  }

 private:
  T value_{};
  std::array<T, N> dvalues_{};
};

/// @brief A number whose value and every derivative are zero, carrying indices `[0, Count)`.
///
/// What a sum over the whole range starts from. Every index is there from the start, so a
/// derivative asked for at an index no term touched reads zero instead of failing to compile,
/// and a sum of no terms at all is zero.
/// @tparam T The scalar type.
/// @tparam Count How many indices, starting at 0.
template <class T, std::size_t Count>
constexpr auto zero() -> dense<T, Count> {
  return {};
}

/// @brief Internal details for reading derivatives out.
namespace details {
template <std::size_t Begin, class X, std::size_t... Ks>
constexpr auto derivatives(const X &x, std::index_sequence<Ks...>) {
  return std::array<typename X::value_type, sizeof...(Ks)>{x.template dvalue<Begin + Ks>()...};
}
}  // namespace details

/// @brief The derivatives of @p x at indices `[Begin, Begin + Count)`, in index order.
///
/// Reads a `number` or a `dense` alike. Every index in the range must be one @p x carries; one it
/// does not is a compile error, the same as asking `dvalue` for it directly.
/// @tparam Begin The first index.
/// @tparam Count How many indices.
/// @param x The number to read.
/// @return The derivatives, as plain scalars.
template <std::size_t Begin, std::size_t Count, class X>
constexpr auto derivatives(const X &x) -> std::array<typename X::value_type, Count> {
  return details::derivatives<Begin>(x, std::make_index_sequence<Count>{});
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_DERIVATIVES_HPP
