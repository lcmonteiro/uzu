/// ===============================================================================================
/// @file
///
/// @brief Defines dual number operations and generic operation templates
/// (unary, binary, transform, broadcast).
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_BASE_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_BASE_HPP

#include <cstddef>
#include <tuple>
#include <type_traits>

#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/number.hpp"

namespace dual {

/// @brief What each kind of operation accepts, as a concept per kind.
///
/// An operator or function is written next to its functor and constrains itself with the concept
/// that pairs with how it applies that functor:
///
///     unary_operation             number_operand
///     binary_operation            number_operands
///     elementwise, unary          array_operand
///     elementwise, binary         elementwise_operands  (array_operands or broadcast_operands)
///
/// A binary rule is symmetric on purpose, so `n - 1.0` and `1.0 - n` both resolve. Two numbers
/// must share their scalar type; a number and a plain `scalar` always combine, the scalar being
/// converted to the number's type first.
template <class T>
concept number_operand = is_number_v<T>;

/// @brief A plain value a number combines with: any arithmetic type but `bool`.
///
/// It is converted to the number's own scalar type before use, so `n * 2` means `n * 2.0` and a
/// `float` number stays a `float` number. `bool` is left out because `n + true` is a mistake far
/// more often than it is a 1.
template <class S>
concept scalar = std::is_arithmetic_v<S> && !std::is_same_v<S, bool>;

template <class T1, class T2>
concept number_operands = (is_number_v<T1> && is_number_v<T2> &&
                           std::is_same_v<typename T1::value_type, typename T2::value_type>) ||
                          (is_number_v<T1> && scalar<T2>) || (scalar<T1> && is_number_v<T2>);

template <class T>
concept array_operand = is_array_v<T>;

template <class T1, class T2>
concept array_operands =
    (is_array_v<T1> && is_array_like_v<T2>) || (is_array_v<T2> && is_array_like_v<T1>);

template <class T1, class T2>
concept broadcast_operands = (is_array_v<T1> && (is_number_v<T2> || scalar<T2>)) ||
                             (is_array_v<T2> && (is_number_v<T1> || scalar<T1>));

template <class T1, class T2>
concept elementwise_operands = array_operands<T1, T2> || broadcast_operands<T1, T2>;

/// @brief One number seen through one index: its value, and its derivative at that index.
template <class T>
struct duo {
  const T &v;  ///< The value.
  const T &d;  ///< The derivative.
};

/// @brief Creates a duo object from a number.
template <std::size_t D, class T, std::size_t... Dn>
inline constexpr auto make_duo(const number<T, Dn...> &n) {
  return duo<T>{n.value(), n.template dvalue<D>()};
}

/// @brief Base class for unary operations on numbers.
template <class Derived>
struct unary_operation {
  /// @brief Applies the unary operation to a number.
  template <class T, std::size_t... D1>
  constexpr auto operator()(const number<T, D1...> &n1) const {
    auto result = number<T, D1...>{};
    update_value(n1.value(), result);
    update_dvalues(indices<D1...>{}, n1, result);
    return result;
  }

 protected:
  /// @brief Retrieves the derived class instance.
  constexpr auto self() const {
    return static_cast<const Derived *>(this);
  }

  /// @brief Updates the value of the result.
  template <class T, class N>
  constexpr void update_value(const T &v1, N &n) const {
    n.value(self()->value(v1));
  }

  /// @brief Updates the derivative value of the result for a specific index.
  template <std::size_t D, class N>
  constexpr void update_dvalue(const N &n1, N &n) const {
    n.template dvalue<D>(self()->dvalue(make_duo<D>(n1)));
  }

 private:
  /// @brief Updates all derivative values in a pack.
  template <std::size_t... Dn, class N>
  constexpr void update_dvalues(const indices<Dn...>, const N &n1, N &n) const {
    (update_dvalue<Dn>(n1, n), ...);
  }
};

/// @brief Helper types for managing indices in binary operations.
template <class N1, class N2>
struct binary_operation_indices {
  using common = indices_intersection_t<N1, N2>;
  using only_1 = indices_difference_t<N1, common>;
  using only_2 = indices_difference_t<N2, common>;
  using result = indices_concat_t<only_1, common, only_2>;
};

/// @brief Base class for binary operations on numbers.
template <class Derived>
struct binary_operation {
  /// @brief Applies the binary operation to a number and a scalar.
  /// @tparam T The number's scalar type, which @p s2 is converted to.
  template <class T, std::size_t... D1, scalar S>
  constexpr auto operator()(const number<T, D1...> &n1, const S &s2) const {
    const auto v2 = static_cast<T>(s2);
    auto result = number<T, D1...>{};
    update_value(n1.value(), v2, result);
    update_dvalues(indices<D1...>{}, n1, v2, result);
    return result;
  }

  /// @brief Applies the binary operation to a scalar and a number.
  /// @tparam T The number's scalar type, which @p s1 is converted to.
  template <scalar S, class T, std::size_t... D2>
  constexpr auto operator()(const S &s1, const number<T, D2...> &n2) const {
    const auto v1 = static_cast<T>(s1);
    auto result = number<T, D2...>{};
    update_value(v1, n2.value(), result);
    update_dvalues(indices<D2...>{}, v1, n2, result);
    return result;
  }

  /// @brief Applies the binary operation to two numbers.
  template <class T, std::size_t... D1, std::size_t... D2>
  constexpr auto operator()(const number<T, D1...> &n1, const number<T, D2...> &n2) const {
    using indices = binary_operation_indices<indices<D1...>, indices<D2...>>;
    using result_type = typename indices::result::template type<number, T>;
    auto result = result_type{};
    update_value(n1.value(), n2.value(), result);
    update_dvalues(typename indices::only_1{}, n1, n2.value(), result);
    update_dvalues(typename indices::only_2{}, n1.value(), n2, result);
    update_dvalues(typename indices::common{}, n1, n2, result);
    return result;
  }

 protected:
  /// @brief Retrieves the derived class instance.
  constexpr auto self() const {
    return static_cast<const Derived *>(this);
  }

  /// @brief Updates the value of the result.
  template <class T, class N>
  constexpr void update_value(const T &v1, const T &v2, N &n) const {
    n.value(self()->value(v1, v2));
  }

  /// @brief Updates the derivative value for a specific index.
  template <std::size_t D, class N1, class N2, class N>
  constexpr void update_dvalue(const N1 &n1, const N2 &n2, N &n) const {
    if constexpr (!is_number_v<N1>) {
      n.template dvalue<D>(self()->dvalue(n1, make_duo<D>(n2)));
    } else if constexpr (!is_number_v<N2>) {
      n.template dvalue<D>(self()->dvalue(make_duo<D>(n1), n2));
    } else {
      n.template dvalue<D>(self()->dvalue(make_duo<D>(n1), make_duo<D>(n2)));
    }
  }

 private:
  /// @brief Updates all derivative values for a pack of indices.
  template <std::size_t... Dn, class N1, class N2, class N>
  constexpr void update_dvalues(const indices<Dn...>, const N1 &n1, const N2 &n2, N &n) const {
    (update_dvalue<Dn>(n1, n2, n), ...);
  }
};

/// @brief @p Op applied element by element: how every operation reaches arrays.
///
/// With one array it maps @p Op over the elements. With two operands, at least one of them a
/// `dual::array`, it pairs elements: two tuple-like operands (a `dual::array`, a `std::array`)
/// pair by position and must be the same length; otherwise the operand that is not tuple-like - a
/// number or a scalar - is paired with every element of the other.
///
/// @p Op is the operation's own functor (`plus`, `sin_operation`, ...), so each element of the
/// result is exactly what the operation gives on that element.
/// @tparam Op The functor to apply, default-constructible and callable on elements.
template <class Op>
struct elementwise {
  /// @brief @p Op of each element of @p c.
  template <class... Ts>
  constexpr auto operator()(const array<Ts...> &c) const {
    return apply(c.indices(), c);
  }

  /// @brief @p Op of each pair of elements of @p c1 and @p c2, or of each element and the one
  /// value that is not an array.
  template <class T1, class T2>
  constexpr auto operator()(const T1 &c1, const T2 &c2) const {
    if constexpr (mismatched<T1, T2>) {
      static_assert(!mismatched<T1, T2>, "element-wise operation on arrays of different lengths");
      return 0;  // only to stop here, after the message above
    } else {
      constexpr std::size_t size =
          std::tuple_size_v<std::conditional_t<is_array_like_v<T1>, T1, T2>>;
      return apply(indices_sequence_t<0, size>{}, c1, c2);
    }
  }

 private:
  /// @brief Whether both operands are arrays and their lengths differ.
  template <class T1, class T2>
  static constexpr bool mismatched = [] {
    if constexpr (is_array_like_v<T1> && is_array_like_v<T2>) {
      return std::tuple_size_v<T1> != std::tuple_size_v<T2>;
    } else {
      return false;
    }
  }();

  /// @brief Element @p I of @p c, or @p c itself if it is a single value to broadcast.
  template <std::size_t I, class T>
  static constexpr auto element(const T &c) -> decltype(auto) {
    if constexpr (is_array_like_v<T>) {
      using std::get;  // std containers by name, dual::array by ADL
      return get<I>(c);
    } else {
      return (c);
    }
  }

  /// @brief @p Op of element @p I of every operand.
  template <std::size_t I, class... Ts>
  static constexpr auto at(const Ts &...operands) {
    return Op{}(element<I>(operands)...);
  }

  /// @brief The array of @p Op at every index, in order.
  template <std::size_t... Is, class... Ts>
  static constexpr auto apply(indices<Is...>, const Ts &...operands) {
    return array{at<Is>(operands...)...};
  }
};

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_BASE_HPP
