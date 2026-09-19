/// @file number.hpp
/// @brief Defines the dual::number class template and its associated utilities.

#pragma once

#include <array>
#include <cstddef>

namespace dual {
/// @brief Represents a scalar value with associated derivative storage.
///
/// @tparam T The underlying scalar type (e.g., float, double, etc.).
/// @tparam Dn Parameter pack of derivative indices used to define associated
/// derivative storage.
template <class T, std::size_t... Dn>
struct number {
  /// @brief Type alias for the underlying scalar type.
  using value_type = T;

  /// @brief Number of derivatives stored.
  static constexpr std::size_t size = sizeof...(Dn);

  /// @brief Default constructor.
  constexpr explicit number() = default;

  /// @brief Constructor initializing the scalar value.
  /// @param init The initial value to store.
  constexpr explicit number(const T &init) : value_{init} {}

  /// @brief Construct from a scalar value.
  /// @param init The initial scalar value.
  ///
  /// Derivatives are default-initialized to 1.
  constexpr explicit number(const T &value, const T dvalue)
      : value_{value}, dvalues_{{((void)Dn, dvalue)...}} {}

  /// @brief Converting constructor between compatible numbers.
  /// @tparam Ds Parameter pack of the source number's derivative indices.
  /// @param other The source number.
  template <std::size_t... Ds>
  constexpr number(const number<T, Ds...> &other) : value_{other.value()} {
    (dvalue<Ds>(other.template dvalue<Ds>()), ...);
  }

  /// @brief Implicit conversion to the stored scalar value.
  /// @return Constant reference to the stored value.
  constexpr operator const T &() const { return value_; }

  /// @brief Returns the stored scalar value.
  /// @return Constant reference to the stored value.
  constexpr auto value() const -> const auto & { return value_; }

  /// @brief Sets the stored scalar value.
  /// @param value The value to store.
  constexpr void value(const T &value) { value_ = value; }

  /// @brief Retrieves the value of the D-th associated derivative.
  /// @tparam D The derivative index to retrieve (must be in Dn...).
  /// @return Constant reference to the stored derivative value.
  template <std::size_t D>
  constexpr auto dvalue() const -> const auto & {
    static_assert(index_of(D) != npos, "index is not tracked by this number");
    constexpr std::size_t at = index_of(D);
    return dvalues_[at];
  }

  /// @brief Sets the value of the D-th associated derivative.
  /// @tparam D The derivative index to set (must be in Dn...).
  /// @param value The value to store for the derivative.
  template <std::size_t D>
  constexpr void dvalue(const T &value) {
    static_assert(index_of(D) != npos, "index is not tracked by this number");
    constexpr std::size_t at = index_of(D);
    dvalues_[at] = value;
  }

 protected:
  /// @brief Marker for "index not tracked by this number".
  static constexpr std::size_t npos = static_cast<std::size_t>(-1);

  /// @brief Storage position of derivative index @p d, or #npos if absent.
  ///
  /// Replaces `std::get<dvalue_type<D>>` on a tuple. Every call site passes a
  /// template parameter and binds the result to a `constexpr` local, so the
  /// scan is folded to a constant and never reaches the generated code.
  ///
  /// Written as a loop rather than `std::find`. `std::find` is `constexpr` as
  /// of C++20 and `dvalue<D>()` above asserts on this result, so the standard
  /// no longer rules it out - it is just slower to compile, by 5% on the
  /// bezier benchmark, measured again after the move to C++20.
  ///
  /// Returns a sentinel rather than a `std::optional` on purpose: the
  /// optional is materialised and unwrapped at every use, and -O2 measured
  /// 8% to 20% slower on this benchmark folding it away, depending on how
  /// the call site consumed it.
  static constexpr std::size_t index_of(std::size_t d) {
    constexpr std::array<std::size_t, sizeof...(Dn)> keys{Dn...};
    for (std::size_t i = 0; i < keys.size(); ++i) {
      if (keys[i] == d) {
        return i;
      }
    }

    return npos;
  }

 private:
  /// @brief Seed given to a derivative that is not explicitly set.
  static constexpr T one = T{1};

  /// @brief The stored scalar value.
  T value_{};

  /// @brief Flat derivative storage, one slot per index in Dn.
  std::array<T, sizeof...(Dn)> dvalues_{((void)Dn, one)...};
};

/// @brief Type trait to detect if a type is a specialization of number<T, S>.
/// @tparam T Type to test.
template <class T>
struct is_number : std::false_type {};

/// @brief Specialization of is_number for number<T, S>.
/// @tparam T Value type.
/// @tparam Ds Derivative index parameter pack.
template <class T, std::size_t... Ds>
struct is_number<number<T, Ds...>> : std::true_type {};

/// @brief Helper variable template for is_number.
/// @tparam T Type to test.
template <class T>
constexpr bool is_number_v = is_number<T>::value;

/// @brief Trait to detect arithmetic types and number specializations.
/// @tparam T Type to test.
template <class T>
struct is_number_like : std::is_arithmetic<T> {};

/// @brief Trait specialization for number<T, Ds...>.
/// @tparam T Value type.
/// @tparam Ds Derivative index parameter pack.
template <class T, std::size_t... Ds>
struct is_number_like<number<T, Ds...>> : std::true_type {};

/// @brief Helper variable template for is_number_like.
/// @tparam T Type to test.
template <class T>
constexpr bool is_number_like_v = is_number_like<T>::value;

}  // namespace dual
