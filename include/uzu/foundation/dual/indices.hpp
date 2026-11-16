/// ===============================================================================================
/// @file
///
/// @brief `dual::indices`, a compile-time list of derivative indices, and what a binary operation
/// needs of two of them: their intersection, their differences, and the three concatenated.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_INDICES_HPP
#define UZU_FOUNDATION_DUAL_INDICES_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

namespace dual {
/// @brief Represents a compile-time sequence of indices.
template <size_t... I>
struct indices {
  /// @brief Generates a type by expanding indices into a template.
  template <template <class, size_t...> class Type, class T>
  using type = Type<T, I...>;
};

/// @brief Concatenates two index sequences.
template <size_t... A, size_t... B>
constexpr auto operator+(indices<A...>, indices<B...>) {
  return indices<A..., B...>{};
}

/// @brief Checks if I is present in the index pack Is.
template <size_t I, size_t... Is>
constexpr bool contains = ((I == Is) || ...);

// ========== FILTER ================
namespace impl {
/// @brief Filters indices by a predicate.
template <class Indices, template <size_t> class Predicate>
struct indices_filter;

/// @brief Specialization for filtering indices.
template <template <size_t> class Predicate, size_t... Is>
struct indices_filter<indices<Is...>, Predicate> {
  /// @brief Wraps each index if predicate is true.
  template <size_t I>
  using wrap_if = std::conditional_t<Predicate<I>::value, indices<I>, indices<>>;

  /// @brief Type with filtered indices.
  using type = decltype((indices<>{} + ... + wrap_if<Is>{}));
};
}  // namespace impl

/// @brief Alias for filtered indices by predicate.
template <class Indices, template <size_t> class Predicate>
using indices_filter_t = typename impl::indices_filter<Indices, Predicate>::type;

// ========== INTERSECTION ==========
namespace impl {
/// @brief Computes intersection of two index sequences.
template <class IndicesA, class IndicesB>
struct intersection;

/// @brief Specialization for intersection.
template <size_t... Ia, size_t... Ib>
struct intersection<indices<Ia...>, indices<Ib...>> {
  /// @brief Checks if index exists in the second sequence.
  template <size_t I>
  using in_b = std::bool_constant<contains<I, Ib...>>;

  /// @brief Type for intersection result.
  using type = indices_filter_t<indices<Ia...>, in_b>;
};
}  // namespace impl

/// @brief Alias for intersection of two index sequences.
template <class IndicesA, class IndicesB>
using indices_intersection_t = typename impl::intersection<IndicesA, IndicesB>::type;

// ========== DIFFERENCE ==========
namespace impl {
/// @brief Computes the difference between two index sequences.
template <class IndicesA, class IndicesB>
struct difference;

/// @brief Specialization for difference.
template <size_t... Ia, size_t... Ib>
struct difference<indices<Ia...>, indices<Ib...>> {
  /// @brief Checks if index is not in the second sequence.
  template <size_t I>
  using not_in_b = std::bool_constant<!contains<I, Ib...>>;

  /// @brief Type for difference result.
  using type = indices_filter_t<indices<Ia...>, not_in_b>;
};
}  // namespace impl

/// @brief Alias for the difference between two index sequences.
template <class IndicesA, class IndicesB>
using indices_difference_t = typename impl::difference<IndicesA, IndicesB>::type;

// ========== CONCAT ==========
namespace impl {
/// @brief Concatenates three index sequences.
template <class Ia, class Ib, class Ic>
struct indices_concat;

/// @brief Specialization for concatenation.
template <size_t... Ia, size_t... Ib, size_t... Ic>
struct indices_concat<indices<Ia...>, indices<Ib...>, indices<Ic...>> {
  /// @brief Type for concatenated indices.
  using type = indices<Ia..., Ib..., Ic...>;
};
}  // namespace impl

/// @brief Alias for concatenation of three index sequences.
template <class IndicesA, class IndicesB, class IndicesC>
using indices_concat_t = typename impl::indices_concat<IndicesA, IndicesB, IndicesC>::type;

// ========== SEQUENCE ==========
namespace impl {
/// @brief Generates a sequence of indices starting from a given value.
template <std::size_t StartSeq, class IndexSeq>
struct indices_sequence;

/// @brief Specialization for generating index sequences.
template <std::size_t Start, std::size_t... Indices>
struct indices_sequence<Start, std::index_sequence<Indices...>> {
  /// @brief Type representing the index sequence.
  using type = indices<(Start + Indices)...>;
};
}  // namespace impl

/// @brief Alias for a sequence of indices starting at Start and of size Size.
template <std::size_t Start, std::size_t Size>
using indices_sequence_t =
    typename impl::indices_sequence<Start, std::make_index_sequence<Size>>::type;

/// @brief Alias for an index sequence for a pack of types, starting at Start.
template <std::size_t Start, class... Ts>
using indices_sequence_for_t = indices_sequence_t<Start, sizeof...(Ts)>;

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_INDICES_HPP
