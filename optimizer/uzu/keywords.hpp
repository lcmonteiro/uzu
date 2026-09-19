/// @file keywords.hpp
/// @brief Named constructor arguments, so that a node or an edge is built from
/// what the values mean rather than from the order they are written in.

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

namespace uzu {
namespace impl {

/// @brief Tags naming each keyword. Only ever used as a type.
struct init_tag;
struct sigma_tag;
struct lr_tag;
struct beta_tag;
struct iterations_tag;

/// @brief One keyword argument: the values, labelled by the keyword's tag.
/// @tparam Tag The keyword this belongs to.
/// @tparam T The scalar type of the values.
/// @tparam N How many values were given.
template <class Tag, class T, std::size_t N>
struct argument {
  using tag_type = Tag;
  using value_type = T;

  static constexpr std::size_t size = N;

  std::array<T, N> values;
};

/// @brief A keyword. Assigning to it produces the argument it names.
///
/// Taking the braced list as a reference to a raw array is what makes
/// `init = {1.0, 1.0}` work: a braced list cannot deduce a std::array, but it
/// binds to `const T (&)[N]` and carries its length with it.
template <class Tag>
struct keyword {
  template <class T, std::size_t N>
  constexpr auto operator=(const T (&values)[N]) const {
    return argument<Tag, T, N>{std::to_array(values)};
  }

  template <class T>
    requires std::is_arithmetic_v<T>
  constexpr auto operator=(T value) const {
    return argument<Tag, T, 1>{{value}};
  }
};

/// @brief Whether a type is one of the arguments a keyword produces.
///
/// Constructors taking keywords are variadic, so without this they would be a
/// better match for a copy than the copy constructor is. Constraining the pack
/// with it - `keyword_argument... Args` - is what keeps them apart.
template <class T>
struct is_argument : std::false_type {};

template <class Tag, class T, std::size_t N>
struct is_argument<argument<Tag, T, N>> : std::true_type {};

template <class T>
concept keyword_argument = is_argument<std::decay_t<T>>::value;

/// @brief Whether an argument carries the given tag.
template <class Tag, class Argument>
constexpr bool tagged = std::is_same_v<Tag, typename std::decay_t<Argument>::tag_type>;

/// @brief Finds the argument carrying Tag among the ones given.
///
/// The recursion is over the argument list of one constructor call, which is
/// as long as the number of keywords, so it never gets deep.
template <class Tag, class T, std::size_t N>
constexpr auto find(const std::array<T, N> &fallback) {
  return fallback;
}

template <class Tag, class T, std::size_t N, class Argument, class... Rest>
constexpr auto find(const std::array<T, N> &fallback, const Argument &head, const Rest &...rest) {
  if constexpr (tagged<Tag, Argument>) {
    static_assert(std::decay_t<Argument>::size == N, "keyword given the wrong number of values");
    (void)fallback;
    return head.values;
  } else {
    return find<Tag>(fallback, rest...);
  }
}

/// @brief Copies @p given into @p N slots, broadcasting a single value across them all.
///
/// What lets `sigma = {2.0}` stand for a whole row of alike sigmas. The count is checked here
/// rather than at the use, so an argument of the wrong width is caught where it is written.
template <class T, std::size_t N, class U, std::size_t M>
constexpr auto broadcast(const std::array<U, M> &given) -> std::array<T, N> {
  static_assert(M == 1 || M == N, "a keyword takes one value per slot, or a single one for all");

  std::array<T, N> out{};
  for (std::size_t i = 0; i < N; ++i) {
    out[i] = static_cast<T>(given[M == 1 ? 0 : i]);
  }
  return out;
}

/// @brief Finds the argument carrying Tag and spreads it over the fallback's width.
template <class Tag, class T, std::size_t N>
constexpr auto find_all(const std::array<T, N> &fallback) {
  return fallback;
}

template <class Tag, class T, std::size_t N, class Argument, class... Rest>
constexpr auto find_all(
    const std::array<T, N> &fallback, const Argument &head, const Rest &...rest) {
  if constexpr (tagged<Tag, Argument>) {
    (void)fallback;
    return broadcast<T, N>(head.values);
  } else {
    return find_all<Tag>(fallback, rest...);
  }
}

/// @brief Finds a single-valued argument carrying Tag, or the fallback.
///
/// The result takes the fallback's type rather than the one written at the
/// call site, so `lr = 1` means the same as `lr = 1.0`. Without the cast the
/// integer would reach a `double` member through a braced initialiser, which
/// is a narrowing conversion and diagnosed as one.
template <class Tag, class T, class... Args>
constexpr auto find_one(T fallback, const Args &...args) {
  return static_cast<T>(find<Tag>(std::array<T, 1>{fallback}, args...)[0]);
}

}  // namespace impl

/// @brief The keywords themselves.
inline constexpr impl::keyword<impl::init_tag> init{};
inline constexpr impl::keyword<impl::sigma_tag> sigma{};
inline constexpr impl::keyword<impl::lr_tag> lr{};
inline constexpr impl::keyword<impl::beta_tag> beta{};
inline constexpr impl::keyword<impl::iterations_tag> iterations{};

}  // namespace uzu
