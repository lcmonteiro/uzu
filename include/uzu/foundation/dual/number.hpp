/// ===============================================================================================
/// @file
///
/// @brief Defines the dual::number class template and its associated utilities.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_NUMBER_HPP
#define UZU_FOUNDATION_DUAL_NUMBER_HPP

#include <array>
#include <cstddef>

namespace dual {
/// @brief A value and its derivatives with respect to the variables `Dn...`, one per index.
template <class T, std::size_t... Dn>
struct number {
  using value_type = T;

  /// @brief How many derivatives it carries: one per index in `Dn`.
  static constexpr std::size_t size = sizeof...(Dn);

  /// @brief A value of zero, every derivative seeded at one.
  constexpr explicit number() = default;

  /// @brief @p value, every derivative seeded at one: a fresh variable for each index.
  constexpr explicit number(const T &value) : value_{value} {
  }

  /// @brief @p value, every derivative set to @p dvalue.
  constexpr explicit number(const T &value, const T dvalue)
      : value_{value}, dvalues_{{((void)Dn, dvalue)...}} {
  }

  /// @brief Widens @p other to this number's indices: its value and its derivatives are copied,
  /// and every index it does not carry reads zero - it does not depend on them.
  ///
  /// Each of @p other's indices must be one this number carries too, so nothing is ever dropped.
  template <std::size_t... Ds>
  constexpr number(const number<T, Ds...> &other) : value_{other.value()}, dvalues_{} {
    (dvalue<Ds>(other.template dvalue<Ds>()), ...);
  }

  /// @brief Adds @p other in place: its value, and each of its derivatives into the slot of the
  /// same index. Only the indices @p other carries are touched, and each must be one this number
  /// carries too.
  template <std::size_t... Ds>
  constexpr auto operator+=(const number<T, Ds...> &other) -> number & {
    value_ += other.value();
    (dvalue<Ds>(dvalue<Ds>() + other.template dvalue<Ds>()), ...);
    return *this;
  }

  /// @brief Subtracts @p other in place, the same way `+=` adds it: only the indices @p other
  /// carries are touched, and each must be one this number carries too.
  template <std::size_t... Ds>
  constexpr auto operator-=(const number<T, Ds...> &other) -> number & {
    value_ -= other.value();
    (dvalue<Ds>(dvalue<Ds>() - other.template dvalue<Ds>()), ...);
    return *this;
  }

  /// @brief Explicit conversion to the stored scalar value. Explicit so that a call such as
  /// `std::sin(n)` is an error rather than a silent conversion that drops the derivatives.
  constexpr explicit operator const T &() const {
    return value_;
  }

  /// @brief Returns the stored scalar value.
  constexpr auto value() const -> const auto & {
    return value_;
  }

  /// @brief Sets the stored scalar value.
  constexpr void value(const T &value) {
    value_ = value;
  }

  /// @brief Retrieves the value of the D-th associated derivative.
  /// @tparam D The derivative index to retrieve (must be in Dn...).
  template <std::size_t D>
  constexpr auto dvalue() const -> const auto & {
    static_assert(index_of(D) != npos, "index is not tracked by this number");
    constexpr std::size_t at = index_of(D);
    return dvalues_[at];
  }

  /// @brief Sets the value of the D-th associated derivative.
  /// @tparam D The derivative index to set (must be in Dn...).
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
  /// Every call site passes a template parameter and binds the result to a `constexpr` local, so
  /// the scan is folded to a constant and never reaches the generated code. A hand-written loop
  /// and a sentinel, rather than `std::find` and `std::optional`, because both compile measurably
  /// slower here; see `docs/dual-storage.md`.
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

/// @brief Whether @p T is a `dual::number`.
template <class T>
struct is_number : std::false_type {};

template <class T, std::size_t... Ds>
struct is_number<number<T, Ds...>> : std::true_type {};

/// @brief Helper variable template for is_number.
template <class T>
constexpr bool is_number_v = is_number<T>::value;

/// @brief Trait to detect arithmetic types and number specializations.
template <class T>
struct is_number_like : std::is_arithmetic<T> {};

/// @brief Trait specialization for number<T, Ds...>.
template <class T, std::size_t... Ds>
struct is_number_like<number<T, Ds...>> : std::true_type {};

/// @brief Helper variable template for is_number_like.
template <class T>
constexpr bool is_number_like_v = is_number_like<T>::value;

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_NUMBER_HPP
