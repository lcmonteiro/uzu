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
/// @brief Variadic array class template to hold a tuple of number<T, S>
/// elements.
/// @tparam Ts Types, each expected to be of form number<T, S>.
template <class... Ts>
struct array {
  /// @brief Compile-time check that ensures all Ts are number<T, S>
  /// specializations.
  static_assert(
      (is_number_v<std::decay_t<Ts>> && ...),
      "All template arguments Ts... must be number<T, S> types.");

  /// @brief Tuple type storing the numbers.
  using storage_t = std::tuple<Ts...>;

  /// @brief Returns the number of elements in the array.
  /// @return Size of the array.
  static constexpr std::size_t size() noexcept {
    return sizeof...(Ts);
  }

  /// @brief Returns an index sequence with offset 0 for all types in the array.
  /// @return Index sequence.
  static constexpr auto indices() noexcept {
    return indices_sequence_for_t<0, Ts...>{};
  }

  /// @brief Default constructor.
  array() = default;

  /// @brief Copy is the tuple's, and both halves of it are spelled out.
  ///
  /// Declaring the assignment alone leaves the copy *constructor* implicit, and an implicit copy
  /// constructor alongside a user-declared assignment is deprecated. Naming both keeps the
  /// behaviour identical and the header warning-free. Move stays suppressed by the user-declared
  /// copy, as it already was: every element is a trivially copyable `number`, so there is nothing
  /// a move would do differently.
  array(const array &) = default;
  array &operator=(const array &) = default;

  /// @brief Constructs array from a parameter pack of number<T, S> values.
  /// @param init Parameter pack of number<T, S> values.
  constexpr explicit array(const Ts &...init) : storage_{init...} {
  }

  /// @brief Constructs an array from a tuple of number<T, S> elements.
  /// @param s Tuple of number<T, S> elements.
  constexpr explicit array(const storage_t &s) : storage_{s} {
  }

  /// @brief Constructs array from a std::array of values.
  /// @tparam U Element type of the input array.
  /// @param init std::array of values to initialize the number<T, S> values.
  template <class U>
  constexpr explicit array(const std::array<U, sizeof...(Ts)> &init) {
    update(init, indices_sequence_for_t<0, Ts...>{});
  }

  /// @brief Constructs array from another dual::array.
  /// @tparam Us Types in the source array.
  /// @param init Source array.
  template <class... Us>
  constexpr explicit array(const array<Us...> &init) {
    update(init, indices_sequence_for_t<0, Ts...>{});
  }

  /// @brief Assigns values from a std::array.
  /// @tparam U Element type.
  /// @param init std::array to assign.
  /// @return Reference to this array.
  template <class U>
  array &operator=(const std::array<U, sizeof...(Ts)> &init) {
    update(init, indices_sequence_for_t<0, Ts...>{});
    return *this;
  }

  /// @brief Assigns values from another dual::array.
  /// @tparam Us Types in the source array.
  /// @param init Source array.
  /// @return Reference to this array.
  template <class... Us>
  array &operator=(const array<Us...> &init) {
    update(init, indices_sequence_for_t<0, Ts...>{});
    return *this;
  }

  /// @brief Access the N-th element in the array (non-const).
  /// @tparam N Index of the element to access.
  /// @return Reference to the N-th element.
  template <std::size_t N>
  constexpr auto &at() noexcept {
    static_assert(N < size(), "Index out of bounds in array::at<N>()");
    return std::get<N>(storage_);
  }

  /// @brief Access the N-th element in the array (const).
  /// @tparam N Index of the element to access.
  /// @return Const reference to the N-th element.
  template <std::size_t N>
  constexpr const auto &at() const noexcept {
    static_assert(N < size(), "Index out of bounds in array::at<N>()");
    return std::get<N>(storage_);
  }

  /// @brief Returns the internal tuple storage (const).
  /// @return Const reference to storage.
  constexpr auto storage() const noexcept -> const storage_t & {
    return storage_;
  }

  /// @brief Returns the internal tuple storage (non-const).
  /// @return Reference to storage.
  constexpr auto storage() noexcept -> storage_t & {
    return storage_;
  }

  /// @brief Converts the array to a std::array by extracting the value() of
  /// each element.
  /// @return std::array of extracted values.
  constexpr auto to_array() const {
    return std::apply([](const auto &...elems) { return std::array{elems.value()...}; }, storage_);
  }

 private:
  storage_t storage_{};

  /// @brief Helper function to initialize storage from a std::array<U, N>.
  /// @tparam U Element type of the input array.
  /// @tparam Is Index sequence to unpack tuple elements.
  /// @param data The input std::array of values.
  /// @param Unused index sequence for unpacking.
  template <class U, std::size_t... Is>
  constexpr void update(const std::array<U, sizeof...(Ts)> &data, dual::indices<Is...>) {
    ((std::get<Is>(storage_).value(std::get<Is>(data))), ...);
  }

  /// @brief Helper function to initialize storage from another array.
  /// @tparam Us Types of elements in the source array.
  /// @tparam Is Index sequence to unpack tuple elements.
  /// @param data Source array.
  template <class... Us, std::size_t... Is>
  constexpr void update(const array<Us...> &data, dual::indices<Is...>) {
    ((std::get<Is>(storage_) = data.template at<Is>()), ...);
  }
};

namespace impl {
/// @brief Type trait to detect if a type is a specialization of array<Ts...>.
/// @tparam T Type to test.
template <class T>
struct is_array : std::false_type {};

/// @brief Specialization of is_array for array<Ts...>.
/// @tparam Ts Value type.
template <class... Ts>
struct is_array<array<Ts...>> : std::true_type {};
}  // namespace impl

/// @brief Helper variable template for is_array.
/// @tparam T Type to test.
template <class T>
constexpr bool is_array_v = impl::is_array<T>::value;

namespace impl {
/// @brief Trait to detect array-like types.
/// @tparam T Type to test.
/// @tparam void SFINAE.
template <class T, class = void>
struct is_array_like : std::false_type {};

/// @brief Specialization for types with tuple_size.
/// @tparam T Type to test.
template <class T>
struct is_array_like<T, std::void_t<decltype(std::tuple_size<T>::value)>> : std::true_type {};
}  // namespace impl

/// @brief Helper variable template for is_array_like.
/// @tparam T Type to test.
template <class T>
constexpr bool is_array_like_v = impl::is_array_like<T>::value;

/// @brief Constructs a dual::array from a value and dimension index pack.
/// @tparam Ds Dimension indices.
/// @tparam T Scalar type.
/// @param init The initial value for each number<T, Ds>.
template <std::size_t... Ds, class T>
inline constexpr auto make_array(indices<Ds...>, const T &init) {
  return array{number<T, Ds>(init)...};
}

/// @brief Constructs a dual::array from a std::array and dimension index pack.
/// @tparam Ds Dimension indices.
/// @tparam T Scalar type.
/// @tparam N Number of elements.
/// @param init std::array of initial values.
template <std::size_t... Ds, class T, std::size_t N>
inline constexpr auto make_array(indices<Ds...>, const std::array<T, N> &init) {
  return array<number<T, Ds>...>{init};
}

/// @brief Constructs a dual::array from a value pack and dimension index pack.
/// @tparam Ds Dimension indices.
/// @tparam Ts Value types corresponding to each dimension.
/// @param init The values to initialize each number<T, Ds>.
template <std::size_t... Ds, class... Ts>
inline constexpr auto make_array(indices<Ds...>, const Ts &...init) {
  return array{number<Ts, Ds>{init}...};
}

/// @brief Constructs a dual::array from a dimension and a std::array.
/// @tparam D Starting dimension index.
/// @tparam T Value type.
/// @tparam N Number of elements.
/// @param init std::array of initial values.
template <std::size_t D, class T, std::size_t N>
inline constexpr auto make_array(const std::array<T, N> &init) {
  return make_array(indices_sequence_t<D, N>{}, init);
}

/// @brief Constructs a dual::array from a dimension and a value pack.
/// @tparam D Starting dimension index.
/// @tparam Ts Value types.
/// @param init The values to initialize each number<T, D + i>.
template <std::size_t D, class... Ts>
inline constexpr auto make_array(const Ts &...init) {
  return make_array(indices_sequence_for_t<D, Ts...>{}, init...);
}

/// @brief Constructs a dual::array of N number<T, D + i> initialized with the
/// same value.
/// @tparam D Starting dimension.
/// @tparam N Number of elements.
/// @tparam T Value type.
/// @param init The value to initialize each element.
template <std::size_t D, std::size_t N, class T>
inline constexpr auto make_array(const T &init) {
  return make_array(indices_sequence_t<D, N>{}, init);
}

}  // namespace dual

namespace std {
/// @brief Specialization of std::tuple_size for dual::array<Ts...>.
/// @tparam Ts Types in the array.
template <typename... Ts>
struct tuple_size<dual::array<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

/// @brief Specialization of std::tuple_element for dual::array<Ts...>.
/// @tparam N Index of the element.
/// @tparam Ts Types in the array.
template <std::size_t N, typename... Ts>
struct tuple_element<N, dual::array<Ts...>> {
  using type = typename tuple_element<N, typename dual::array<Ts...>::storage_t>::type;
};

/// @brief Enables std::get<N>(dual::array<Ts...>) (const version).
/// @tparam N Index.
/// @tparam Ts Element types.
/// @param a The dual::array instance.
/// @return const reference to the N-th element.
template <std::size_t N, typename... Ts>
constexpr const auto &get(const dual::array<Ts...> &a) noexcept {
  return std::get<N>(a.storage());
}

/// @brief Enables std::get<N>(dual::array<Ts...>) (non-const version).
/// @tparam N Index.
/// @tparam Ts Element types.
/// @param a The dual::array instance.
/// @return reference to the N-th element.
template <std::size_t N, typename... Ts>
constexpr auto &get(dual::array<Ts...> &a) noexcept {
  return std::get<N>(a.storage());
}

/// @brief Enables std::get<N>(dual::array<Ts...>) (rvalue version).
/// @tparam N Index.
/// @tparam Ts Element types.
/// @param a The dual::array instance (rvalue).
/// @return rvalue reference to the N-th element.
template <std::size_t N, typename... Ts>
constexpr auto &&get(dual::array<Ts...> &&a) noexcept {
  return std::get<N>(std::move(a.storage()));
}

}  // namespace std

#endif  // UZU_FOUNDATION_DUAL_ARRAY_HPP
