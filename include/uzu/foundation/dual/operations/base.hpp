/// ===============================================================================================
/// @file
///
/// @brief Defines dual number operations and generic operation templates
/// (unary, binary, transform, broadcast).
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_OPERATIONS_BASE_HPP
#define UZU_FOUNDATION_DUAL_OPERATIONS_BASE_HPP

#include "uzu/foundation/dual/array.hpp"
#include "uzu/foundation/dual/number.hpp"

namespace dual {

/// @brief What each operation base accepts, as a concept per base.
///
/// These replace the `enable_if` each base used to publish as a member alias. An operator or
/// `std::` overload is written next to its functor and constrains itself with the concept that
/// pairs with that functor's base:
///
///     unary_operation             number_operand
///     binary_operation            number_operands
///     transform_unary_operation   array_operand
///     transform_binary_operation  array_operands
///     broadcast_operation         broadcast_operands
///
/// A binary rule is symmetric on purpose: one side has to be the real thing and the other only
/// has to be convertible to it, so `n - 1.0` and `1.0 - n` both resolve.
template <class T>
concept number_operand = is_number_v<T>;

template <class T1, class T2>
concept number_operands =
    (is_number_v<T1> && is_number_like_v<T2>) || (is_number_v<T2> && is_number_like_v<T1>);

template <class T>
concept array_operand = is_array_v<T>;

template <class T1, class T2>
concept array_operands =
    (is_array_v<T1> && is_array_like_v<T2>) || (is_array_v<T2> && is_array_like_v<T1>);

template <class T1, class T2>
concept broadcast_operands =
    (is_array_v<T1> && is_number_like_v<T2>) || (is_array_v<T2> && is_number_like_v<T1>);

/// @brief Structure representing a dual number with a value and derivative.
/// @tparam T The data type of the value and derivative.
template <class T>
struct duo {
  const T &v;  ///< The value.
  const T &d;  ///< The derivative.
};

/// @brief Creates a duo object from a number.
/// @tparam D The differentiation order.
/// @tparam T The data type of the number.
/// @tparam Dn Additional differentiation orders.
/// @param n The number object.
/// @return A duo object containing the value and derivative.
template <std::size_t D, class T, std::size_t... Dn>
inline constexpr auto make_duo(const number<T, Dn...> &n) {
  return duo<T>{n.value(), n.template dvalue<D>()};
}

/// @brief Base class for unary operations on numbers.
/// @tparam Derived The derived class implementing the operation.
template <class Derived>
struct unary_operation {
  /// @brief Applies the unary operation to a number.
  /// @tparam T The data type of the number.
  /// @tparam D1 Differentiation orders.
  /// @param n1 The input number.
  /// @return The result of the operation.
  template <class T, std::size_t... D1>
  constexpr auto operator()(const number<T, D1...> &n1) const {
    auto result = number<T, D1...>{};
    update_value(n1.value(), result);
    update_dvalues(indices<D1...>{}, n1, result);
    return result;
  }

 protected:
  /// @brief Retrieves the derived class instance.
  /// @return Pointer to the derived class.
  constexpr auto self() const {
    return static_cast<const Derived *>(this);
  }

  /// @brief Updates the value of the result.
  /// @tparam T Input value type.
  /// @tparam N Result number type.
  /// @param v1 The input value.
  /// @param n The result number.
  template <class T, class N>
  constexpr void update_value(const T &v1, N &n) const {
    n.value(self()->value(v1));
  }

  /// @brief Updates the derivative value of the result for a specific index.
  /// @tparam D The differentiation order.
  /// @tparam N The number type.
  /// @param n1 The input number.
  /// @param n The result number.
  template <std::size_t D, class N>
  constexpr void update_dvalue(const N &n1, N &n) const {
    n.template dvalue<D>(self()->dvalue(make_duo<D>(n1)));
  }

 private:
  /// @brief Updates all derivative values in a pack.
  /// @tparam Dn Differentiation orders.
  /// @tparam N The number type.
  /// @param n1 The input number.
  /// @param n The result number.
  template <std::size_t... Dn, class N>
  constexpr void update_dvalues(const indices<Dn...>, const N &n1, N &n) const {
    (update_dvalue<Dn>(n1, n), ...);
  }
};

/// @brief Helper types for managing indices in binary operations.
template <class N1, class N2>
struct binary_operation_indices {
  using common = indices_interception_t<N1, N2>;
  using only_1 = indices_difference_t<N1, common>;
  using only_2 = indices_difference_t<N2, common>;
  using result = indices_concat_t<only_1, common, only_2>;
};

/// @brief Base class for binary operations on numbers.
/// @tparam Derived The derived class implementing the operation.
template <class Derived>
struct binary_operation {
  /// @brief Applies the binary operation to a number and a scalar.
  /// @tparam T The data type of the number.
  /// @tparam D1 Differentiation orders.
  /// @param n1 The input number.
  /// @param v2 The scalar value.
  /// @return The result of the operation.
  template <class T, std::size_t... D1>
  constexpr auto operator()(const number<T, D1...> &n1, const T &v2) const {
    auto result = number<T, D1...>{};
    update_value(n1.value(), v2, result);
    update_dvalues(indices<D1...>{}, n1, v2, result);
    return result;
  }

  /// @brief Applies the binary operation to a scalar and a number.
  /// @tparam T The data type of the number.
  /// @tparam D2 Differentiation orders.
  /// @param v1 The scalar value.
  /// @param n2 The input number.
  /// @return The result of the operation.
  template <class T, std::size_t... D2>
  constexpr auto operator()(const T &v1, const number<T, D2...> &n2) const {
    auto result = number<T, D2...>{};
    update_value(v1, n2.value(), result);
    update_dvalues(indices<D2...>{}, v1, n2, result);
    return result;
  }

  /// @brief Applies the binary operation to two numbers.
  /// @tparam T The data type of the numbers.
  /// @tparam D1 Differentiation orders for n1.
  /// @tparam D2 Differentiation orders for n2.
  /// @param n1 First input number.
  /// @param n2 Second input number.
  /// @return The result of the operation.
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
  /// @return Pointer to the derived class.
  constexpr auto self() const {
    return static_cast<const Derived *>(this);
  }

  /// @brief Updates the value of the result.
  /// @tparam T Input value type.
  /// @tparam N Result number type.
  /// @param v1 First input value.
  /// @param v2 Second input value.
  /// @param n The result number.
  template <class T, class N>
  constexpr void update_value(const T &v1, const T &v2, N &n) const {
    n.value(self()->value(v1, v2));
  }

  /// @brief Updates the derivative value for a specific index.
  /// @tparam D Differentiation order.
  /// @tparam N1 First operand type.
  /// @tparam N2 Second operand type.
  /// @tparam N Result type.
  /// @param n1 First input operand.
  /// @param n2 Second input operand.
  /// @param n Result number.
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
  /// @tparam Dn Differentiation orders.
  /// @tparam N1 First operand type.
  /// @tparam N2 Second operand type.
  /// @tparam N Result type.
  /// @param n1 First input operand.
  /// @param n2 Second input operand.
  /// @param n Result number.
  template <std::size_t... Dn, class N1, class N2, class N>
  constexpr void update_dvalues(const indices<Dn...>, const N1 &n1, const N2 &n2, N &n) const {
    (update_dvalue<Dn>(n1, n2, n), ...);
  }
};

/// @brief Base class for element-wise unary operations on arrays.
/// @tparam Derived The derived class implementing the operation.
template <class Derived>
struct transform_unary_operation {
  /// @brief Applies the unary operation to an array.
  /// @param c Input array.
  /// @return Result array after transformation.
  template <class... T>
  constexpr auto operator()(const array<T...> &c) const {
    return transform(c, c.indices());
  }

 protected:
  /// @brief Retrieves the derived class instance.
  constexpr auto self() const {
    return static_cast<const Derived *>(this);
  }

  /// @brief Helper for transforming each element of the array.
  /// @tparam T Input array type.
  /// @tparam Is Index sequence.
  /// @param c Input array.
  /// @return Transformed array.
  template <class T, std::size_t... Is>
  constexpr auto transform(const T &c, indices<Is...>) const {
    return array{self()->transform(std::get<Is>(c))...};
  }
};

/// @brief Base class for element-wise binary operations on arrays.
/// @tparam Derived The derived class implementing the operation.
template <class Derived>
struct transform_binary_operation {
  /// @brief Applies the binary operation to two arrays or array-like objects.
  /// @param c1 First array.
  /// @param c2 Second array or array-like object.
  /// @return Result array after transformation.
  template <class... T1, class T2>
  constexpr auto operator()(const array<T1...> &c1, const T2 &c2) const {
    return transform(c1, c2, c1.indices());
  }

  template <class T1, class... T2>
  constexpr auto operator()(const T1 &c1, const array<T2...> &c2) const {
    return transform(c1, c2, c2.indices());
  }

  template <class... T1, class... T2>
  constexpr auto operator()(const array<T1...> &c1, const array<T2...> &c2) const {
    return transform(c1, c2, c2.indices());
  }

 protected:
  /// @brief Retrieves the derived class instance.
  constexpr auto self() const {
    return static_cast<const Derived *>(this);
  }

  /// @brief Helper for transforming each element of two arrays.
  /// @tparam T1 First array type.
  /// @tparam T2 Second array type.
  /// @tparam Is Index sequence.
  /// @param c1 First array.
  /// @param c2 Second array.
  /// @return Transformed array.
  template <class T1, class T2, std::size_t... Is>
  constexpr auto transform(const T1 &c1, const T2 &c2, indices<Is...>) const {
    return array{self()->transform(std::get<Is>(c1), std::get<Is>(c2))...};
  }
};

/// @brief Base class for broadcasting operations on arrays and scalars.
/// @tparam Derived The derived class implementing the operation.
template <class Derived>
struct broadcast_operation {
  /// @brief Applies the broadcast operation to an array and a scalar.
  /// @param c1 Array input.
  /// @param c2 Scalar input.
  /// @return Result array after transformation.
  template <class... T1, class T2>
  constexpr auto operator()(const array<T1...> &c1, const T2 &c2) const {
    return transform(c1, c2, c1.indices());
  }

  /// @brief Applies the broadcast operation to a scalar and an array.
  /// @param c1 Scalar input.
  /// @param c2 Array input.
  /// @return Result array after transformation.
  template <class T1, class... T2>
  constexpr auto operator()(const T1 &c1, const array<T2...> &c2) const {
    return transform(c1, c2, c2.indices());
  }

 protected:
  /// @brief Retrieves the derived class instance.
  constexpr auto self() const {
    return static_cast<const Derived *>(this);
  }

  /// @brief Helper for applying the operation across all elements of an array
  /// and a scalar.
  /// @tparam T1 Array type.
  /// @tparam T2 Scalar type.
  /// @tparam Is Index sequence.
  /// @param c1 Array input.
  /// @param c2 Scalar input.
  /// @return Result array after transformation.
  template <class... T1, class T2, std::size_t... Is>
  constexpr auto transform(const array<T1...> &c1, const T2 &c2, indices<Is...>) const {
    return array{self()->transform(std::get<Is>(c1), c2)...};
  }

  /// @brief Helper for applying the operation across all elements of a scalar
  /// and an array.
  /// @tparam T1 Scalar type.
  /// @tparam T2 Array type.
  /// @tparam Is Index sequence.
  /// @param c1 Scalar input.
  /// @param c2 Array input.
  /// @return Result array after transformation.
  template <class T1, class... T2, std::size_t... Is>
  constexpr auto transform(const T1 &c1, const array<T2...> &c2, indices<Is...>) const {
    return array{self()->transform(c1, std::get<Is>(c2))...};
  }
};

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_OPERATIONS_BASE_HPP
