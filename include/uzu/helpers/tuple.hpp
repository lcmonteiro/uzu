/// ===============================================================================================
/// @file
///
/// @brief `uzu::tuple`: a fixed list of values that is cheap for the compiler at any length.
///
/// It holds what `std::tuple` holds and answers `get<I>`, `std::tuple_size`,
/// `std::tuple_element`, structured bindings and `apply`, but it is built differently, and the
/// difference is the whole reason it exists.
///
/// `std::tuple` (as libstdc++ and libc++ write it) is a recursion: element `i` is a base that
/// carries every element after it in its template arguments, so defining a tuple of N touches
/// O(N^2) template arguments, every `std::get<I>` walks the chain, and every constructor is
/// guarded by constraints evaluated across the whole pack. None of that shows at run time, but
/// it shows in the compiler: a tuple of 400 elements, built and read once, takes 24 s of GCC's
/// time; this one takes 0.16 s. At 800, the `std::tuple` exhausted the compiler's memory.
///
/// Here every element is a direct base, `tuple_leaf<I, T>`, carrying only its own index and type,
/// and all of them are made in one pack expansion. Reaching one is a conversion to a known base:
/// `get<I>` deduces the type from the base itself, without instantiating anything per element.
/// The graph keeps its nodes and edges in one, which is what lets a graph of hundreds of edges
/// compile in reasonable time.
///
/// What it does not have, because nothing here has needed it: comparison operators, `get<T>` by
/// type, `tuple_cat`, converting or allocator constructors, and the empty-member layout
/// optimisation.
/// ===============================================================================================
#ifndef UZU_HELPERS_TUPLE_HPP
#define UZU_HELPERS_TUPLE_HPP

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace uzu {
namespace impl {

/// @brief One element of a `uzu::tuple`: a value, tagged with its position so that two elements
/// of the same type are still two different bases.
template <std::size_t I, class T>
struct tuple_leaf {
  T value{};
};

template <class Indices, class... Ts>
struct tuple_base;

/// @brief Every element as a direct base, made in one expansion.
template <std::size_t... Is, class... Ts>
struct tuple_base<std::index_sequence<Is...>, Ts...> : tuple_leaf<Is, Ts>... {
  constexpr tuple_base() = default;

  constexpr explicit tuple_base(const Ts &...values)
    requires(sizeof...(Ts) > 0)
      : tuple_leaf<Is, Ts>{values}... {
  }
};

/// @brief The type of the element at @p I. Declared only: it exists to be named in `decltype`,
/// where deducing `T` from the base is the whole lookup.
template <std::size_t I, class T>
auto type_at(const tuple_leaf<I, T> &) -> T;

}  // namespace impl

/// @brief A fixed list of values, flat rather than recursive. See the file comment for why.
template <class... Ts>
struct tuple : impl::tuple_base<std::index_sequence_for<Ts...>, Ts...> {
  using impl::tuple_base<std::index_sequence_for<Ts...>, Ts...>::tuple_base;

  /// @brief How many elements it holds.
  static constexpr auto size() -> std::size_t {
    return sizeof...(Ts);
  }
};

template <class... Ts>
tuple(Ts...) -> tuple<Ts...>;

/// @brief The element at @p I. Called as `get<I>(t)`: `T` is deduced from the one base of `t`
/// whose index is `I`.
template <std::size_t I, class T>
constexpr auto get(impl::tuple_leaf<I, T> &leaf) -> T & {
  return leaf.value;
}

template <std::size_t I, class T>
constexpr auto get(const impl::tuple_leaf<I, T> &leaf) -> const T & {
  return leaf.value;
}

template <std::size_t I, class T>
constexpr auto get(impl::tuple_leaf<I, T> &&leaf) -> T && {
  return static_cast<T &&>(leaf.value);
}

namespace impl {

template <class F, class Tuple, std::size_t... Is>
constexpr decltype(auto) apply(F &&f, Tuple &&t, std::index_sequence<Is...>) {
  return static_cast<F &&>(f)(uzu::get<Is>(static_cast<Tuple &&>(t))...);
}

template <class F, class Tuple, std::size_t... Is>
constexpr void for_each(Tuple &&t, F &&f, std::index_sequence<Is...>) {
  // An initializer list rather than a comma fold: evaluated left to right all the same, and flat
  // at any length, where a fold nests as deep as there are elements and clang refuses past 256.
  // What `f` returns is cast to void first, so the comma is always the built-in one: a result
  // type that overloads `operator,` would otherwise have that overload called here instead.
  [[maybe_unused]] const std::array<int, sizeof...(Is)> in_order{
      (static_cast<void>(f(uzu::get<Is>(static_cast<Tuple &&>(t)))), 0)...};
}

}  // namespace impl

/// @brief Calls @p f with the elements of @p t as its arguments, in order.
template <class F, class Tuple>
constexpr decltype(auto) apply(F &&f, Tuple &&t) {
  constexpr auto n = std::remove_cvref_t<Tuple>::size();
  return impl::apply(static_cast<F &&>(f), static_cast<Tuple &&>(t), std::make_index_sequence<n>{});
}

/// @brief Calls @p f once with each element of @p t, first to last.
template <class Tuple, class F>
constexpr void for_each(Tuple &&t, F &&f) {
  constexpr auto n = std::remove_cvref_t<Tuple>::size();
  impl::for_each(static_cast<Tuple &&>(t), f, std::make_index_sequence<n>{});
}

}  // namespace uzu

/// @brief What makes `uzu::tuple` a tuple to the rest of the language: structured bindings, and
/// anything that asks `std::tuple_size` or `std::tuple_element`.
template <class... Ts>
struct std::tuple_size<uzu::tuple<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <std::size_t I, class... Ts>
struct std::tuple_element<I, uzu::tuple<Ts...>> {
  static_assert(I < sizeof...(Ts), "index out of range for this uzu::tuple");
  using type = decltype(uzu::impl::type_at<I>(std::declval<const uzu::tuple<Ts...> &>()));
};

#endif  // UZU_HELPERS_TUPLE_HPP
