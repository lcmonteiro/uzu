/// @file indices.hpp
/// @brief Utilities for index sequence manipulation in template
/// metaprogramming.Provides types and operations for combining, filtering,
/// intersecting, and creating index sequences.

#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace dual {
/// @brief Represents a compile-time sequence of indices.
/// @tparam I Index values.
template <size_t... I>
struct indices {
  /// @brief Generates a type by expanding indices into a template.
  /// @tparam Type The template to be instantiated.
  /// @tparam T The base type for expansion.
  template <template <class, size_t...> class Type, class T>
  using type = Type<T, I...>;
};

/// @brief Concatenates two index sequences.
/// @tparam A Indices of the first sequence.
/// @tparam B Indices of the second sequence.
/// @return Concatenated indices.
template <size_t... A, size_t... B>
constexpr auto operator+(indices<A...>, indices<B...>) {
  return indices<A..., B...>{};
}

/// @brief Checks if I is present in the index pack Is.
/// @tparam I Index to check.
/// @tparam Is Index pack.
template <size_t I, size_t... Is>
constexpr bool contains = ((I == Is) || ...);

// ========== FILTER ================
namespace impl {
/// @brief Filters indices by a predicate.
/// @tparam Indices The sequence to filter.
/// @tparam Predicate Predicate to apply.
template <class Indices, template <size_t> class Predicate>
struct indices_filter;

/// @brief Specialization for filtering indices.
/// @tparam Predicate Predicate to apply.
/// @tparam Is Indices to filter.
template <template <size_t> class Predicate, size_t... Is>
struct indices_filter<indices<Is...>, Predicate> {
  /// @brief Wraps each index if predicate is true.
  /// @tparam I Index to wrap.
  template <size_t I>
  using wrap_if =
      std::conditional_t<Predicate<I>::value, indices<I>, indices<>>;

  /// @brief Type with filtered indices.
  using type = decltype((indices<>{} + ... + wrap_if<Is>{}));
};
}  // namespace impl

/// @brief Alias for filtered indices by predicate.
/// @tparam Indices Indices to filter.
/// @tparam Predicate Predicate to apply.
template <class Indices, template <size_t> class Predicate>
using indices_filter_t =
    typename impl::indices_filter<Indices, Predicate>::type;

// ========== INTERSECTION ==========
namespace impl {
/// @brief Computes intersection of two index sequences.
/// @tparam IndicesA First sequence.
/// @tparam IndicesB Second sequence.
template <class IndicesA, class IndicesB>
struct intersection;

/// @brief Specialization for intersection.
/// @tparam Ia Indices of the first sequence.
/// @tparam Ib Indices of the second sequence.
template <size_t... Ia, size_t... Ib>
struct intersection<indices<Ia...>, indices<Ib...>> {
  /// @brief Checks if index exists in the second sequence.
  /// @tparam I Index to check.
  template <size_t I>
  using in_b = std::bool_constant<contains<I, Ib...>>;

  /// @brief Type for intersection result.
  using type = indices_filter_t<indices<Ia...>, in_b>;
};
}  // namespace impl

/// @brief Alias for intersection of two index sequences.
/// @tparam IndicesA First sequence.
/// @tparam IndicesB Second sequence.
template <class IndicesA, class IndicesB>
using indices_interception_t =
    typename impl::intersection<IndicesA, IndicesB>::type;

// ========== DIFFERENCE ==========
namespace impl {
/// @brief Computes the difference between two index sequences.
/// @tparam IndicesA First sequence.
/// @tparam IndicesB Second sequence.
template <class IndicesA, class IndicesB>
struct difference;

/// @brief Specialization for difference.
/// @tparam Ia Indices of the first sequence.
/// @tparam Ib Indices of the second sequence.
template <size_t... Ia, size_t... Ib>
struct difference<indices<Ia...>, indices<Ib...>> {
  /// @brief Checks if index is not in the second sequence.
  /// @tparam I Index to check.
  template <size_t I>
  using not_in_b = std::bool_constant<!contains<I, Ib...>>;

  /// @brief Type for difference result.
  using type = indices_filter_t<indices<Ia...>, not_in_b>;
};
}  // namespace impl

/// @brief Alias for the difference between two index sequences.
/// @tparam IndicesA First sequence.
/// @tparam IndicesB Second sequence.
template <class IndicesA, class IndicesB>
using indices_difference_t =
    typename impl::difference<IndicesA, IndicesB>::type;

// ========== CONCAT ==========
namespace impl {
/// @brief Concatenates three index sequences.
/// @tparam Ia First sequence.
/// @tparam Ib Second sequence.
/// @tparam Ic Third sequence.
template <class Ia, class Ib, class Ic>
struct indices_concat;

/// @brief Specialization for concatenation.
/// @tparam Ia Indices of the first sequence.
/// @tparam Ib Indices of the second sequence.
/// @tparam Ic Indices of the third sequence.
template <size_t... Ia, size_t... Ib, size_t... Ic>
struct indices_concat<indices<Ia...>, indices<Ib...>, indices<Ic...>> {
  /// @brief Type for concatenated indices.
  using type = indices<Ia..., Ib..., Ic...>;
};
}  // namespace impl

/// @brief Alias for concatenation of three index sequences.
/// @tparam IndicesA First sequence.
/// @tparam IndicesB Second sequence.
/// @tparam IndicesC Third sequence.
template <class IndicesA, class IndicesB, class IndicesC>
using indices_concat_t =
    typename impl::indices_concat<IndicesA, IndicesB, IndicesC>::type;

// ========== SEQUENCE ==========
namespace impl {
/// @brief Generates a sequence of indices starting from a given value.
/// @tparam StartSeq Starting value.
/// @tparam IndexSeq A std::index_sequence to use for offset.
template <std::size_t StartSeq, class IndexSeq>
struct indices_sequence;

/// @brief Specialization for generating index sequences.
/// @tparam Start Starting value.
/// @tparam Indices Values for expansion.
template <std::size_t Start, std::size_t... Indices>
struct indices_sequence<Start, std::index_sequence<Indices...>> {
  /// @brief Type representing the index sequence.
  using type = indices<(Start + Indices)...>;
};
}  // namespace impl

/// @brief Alias for a sequence of indices starting at Start and of size Size.
/// @tparam Start Starting value.
/// @tparam Size Sequence size.
template <std::size_t Start, std::size_t Size>
using indices_sequence_t =
    typename impl::indices_sequence<Start,
                                    std::make_index_sequence<Size>>::type;

/// @brief Alias for an index sequence for a pack of types, starting at Start.
/// @tparam Start Starting value.
/// @tparam Ts Types in the pack.
template <std::size_t Start, class... Ts>
using indices_sequence_for_t = indices_sequence_t<Start, sizeof...(Ts)>;

}  // namespace dual