/// ===============================================================================================
/// @file
///
/// @brief Defines the dual::array class template and its associated utilities.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_ARRAY_HPP
#define UZU_FOUNDATION_DUAL_ARRAY_HPP

#include <array>
#include <tuple>

#include "uzu/foundation/dual/indices.hpp"
#include "uzu/foundation/dual/number.hpp"

namespace dual {
/// @brief A fixed list of dual numbers, each of its own type: element `i` is usually the number
/// for variable `i`, so the types differ in the indices they carry.
template <class... Ts>
struct array {
  static_assert(
      (is_number_v<std::decay_t<Ts>> && ...),
      "every element of a dual::array must be a dual::number");

  /// @brief Tuple type storing the numbers.
  using storage_t = std::tuple<Ts...>;

  /// @brief Returns the number of elements in the array.
  static constexpr std::size_t size() noexcept {
    return sizeof...(Ts);
  }

  /// @brief Returns an index sequence with offset 0 for all types in the array.
  static constexpr auto indices() noexcept {
    return indices_sequence_for_t<0, Ts...>{};
  }

  /// @brief Default constructor.
  array() = default;

  /// @brief From its elements.
  constexpr explicit array(const Ts &...values) : storage_{values...} {
  }

  /// @brief From a tuple of its elements.
  constexpr explicit array(const storage_t &s) : storage_{s} {
  }

  /// @brief Constructs array from a std::array of values.
  template <class U>
  constexpr explicit array(const std::array<U, sizeof...(Ts)> &values) {
    update(values, indices_sequence_for_t<0, Ts...>{});
  }

  /// @brief Constructs array from another dual::array.
  template <class... Us>
  constexpr explicit array(const array<Us...> &values) {
    update(values, indices_sequence_for_t<0, Ts...>{});
  }

  /// @brief Assigns values from a std::array.
  template <class U>
  array &operator=(const std::array<U, sizeof...(Ts)> &values) {
    update(values, indices_sequence_for_t<0, Ts...>{});
    return *this;
  }

  /// @brief Assigns values from another dual::array.
  template <class... Us>
  array &operator=(const array<Us...> &values) {
    update(values, indices_sequence_for_t<0, Ts...>{});
    return *this;
  }

  /// @brief Access the N-th element in the array (non-const).
  template <std::size_t N>
  constexpr auto &at() noexcept {
    static_assert(N < size(), "Index out of bounds in array::at<N>()");
    return std::get<N>(storage_);
  }

  /// @brief Access the N-th element in the array (const).
  template <std::size_t N>
  constexpr const auto &at() const noexcept {
    static_assert(N < size(), "Index out of bounds in array::at<N>()");
    return std::get<N>(storage_);
  }

  /// @brief Returns the internal tuple storage (const).
  constexpr auto storage() const noexcept -> const storage_t & {
    return storage_;
  }

  /// @brief Returns the internal tuple storage (non-const).
  constexpr auto storage() noexcept -> storage_t & {
    return storage_;
  }

  /// @brief Converts the array to a std::array by extracting the value() of
  /// each element.
  constexpr auto to_array() const {
    return std::apply([](const auto &...elems) { return std::array{elems.value()...}; }, storage_);
  }

 private:
  storage_t storage_{};

  /// @brief Helper function to initialize storage from a std::array<U, N>.
  template <class U, std::size_t... Is>
  constexpr void update(const std::array<U, sizeof...(Ts)> &data, dual::indices<Is...>) {
    ((std::get<Is>(storage_).value(std::get<Is>(data))), ...);
  }

  /// @brief Helper function to initialize storage from another array.
  template <class... Us, std::size_t... Is>
  constexpr void update(const array<Us...> &data, dual::indices<Is...>) {
    ((std::get<Is>(storage_) = data.template at<Is>()), ...);
  }
};

namespace impl {
/// @brief Type trait to detect if a type is a specialization of array<Ts...>.
template <class T>
struct is_array : std::false_type {};

/// @brief Specialization of is_array for array<Ts...>.
template <class... Ts>
struct is_array<array<Ts...>> : std::true_type {};
}  // namespace impl

/// @brief Helper variable template for is_array.
template <class T>
constexpr bool is_array_v = impl::is_array<T>::value;

namespace impl {
/// @brief Trait to detect array-like types.
template <class T, class = void>
struct is_array_like : std::false_type {};

/// @brief Specialization for types with tuple_size.
template <class T>
struct is_array_like<T, std::void_t<decltype(std::tuple_size<T>::value)>> : std::true_type {};
}  // namespace impl

/// @brief Helper variable template for is_array_like.
template <class T>
constexpr bool is_array_like_v = impl::is_array_like<T>::value;

/// @brief Constructs a dual::array from a value and dimension index pack.
template <std::size_t... Ds, class T>
inline constexpr auto make_array(indices<Ds...>, const T &value) {
  return array{number<T, Ds>(value)...};
}

/// @brief Constructs a dual::array from a std::array and dimension index pack.
template <std::size_t... Ds, class T, std::size_t N>
inline constexpr auto make_array(indices<Ds...>, const std::array<T, N> &values) {
  return array<number<T, Ds>...>{values};
}

/// @brief Constructs a dual::array from a value pack and dimension index pack.
template <std::size_t... Ds, class... Ts>
inline constexpr auto make_array(indices<Ds...>, const Ts &...values) {
  return array{number<Ts, Ds>{values}...};
}

/// @brief Constructs a dual::array from a dimension and a std::array.
template <std::size_t D, class T, std::size_t N>
inline constexpr auto make_array(const std::array<T, N> &values) {
  return make_array(indices_sequence_t<D, N>{}, values);
}

/// @brief Constructs a dual::array from a dimension and a value pack.
template <std::size_t D, class... Ts>
inline constexpr auto make_array(const Ts &...values) {
  return make_array(indices_sequence_for_t<D, Ts...>{}, values...);
}

/// @brief Constructs a dual::array of N number<T, D + i> initialized with the
/// same value.
template <std::size_t D, std::size_t N, class T>
inline constexpr auto make_array(const T &value) {
  return make_array(indices_sequence_t<D, N>{}, value);
}

/// @brief Enables get<N>(array), found by ADL (const version).
template <std::size_t N, typename... Ts>
constexpr const auto &get(const dual::array<Ts...> &a) noexcept {
  return std::get<N>(a.storage());
}

/// @brief Enables get<N>(array), found by ADL (non-const version).
template <std::size_t N, typename... Ts>
constexpr auto &get(dual::array<Ts...> &a) noexcept {
  return std::get<N>(a.storage());
}

/// @brief Enables get<N>(array), found by ADL (rvalue version).
template <std::size_t N, typename... Ts>
constexpr auto &&get(dual::array<Ts...> &&a) noexcept {
  return std::get<N>(std::move(a.storage()));
}

}  // namespace dual

namespace std {
/// @brief Specialization of std::tuple_size for dual::array<Ts...>.
template <typename... Ts>
struct tuple_size<dual::array<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

/// @brief Specialization of std::tuple_element for dual::array<Ts...>.
template <std::size_t N, typename... Ts>
struct tuple_element<N, dual::array<Ts...>> {
  using type = typename tuple_element<N, typename dual::array<Ts...>::storage_t>::type;
};
}  // namespace std

#endif  // UZU_FOUNDATION_DUAL_ARRAY_HPP
