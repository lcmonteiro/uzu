/// ===============================================================================================
/// @file
///
/// @brief The Cartesian product of tuple-like containers: every combination of one element from
/// each, as a tuple of rows the folds in this directory can walk.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_PRODUCT_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_PRODUCT_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>

namespace dual {

/// @brief Internal details for building the product.
namespace details {

/// @brief Where row @p row of the product reads container @p which, for containers of sizes
/// `Sizes...`. Rows are in row-major order: the last container varies fastest.
template <std::size_t... Sizes>
constexpr auto product_index(std::size_t row, std::size_t which) -> std::size_t {
  constexpr std::array<std::size_t, sizeof...(Sizes)> sizes{Sizes...};
  std::size_t stride = 1;
  for (std::size_t i = which + 1; i < sizes.size(); ++i) {
    stride *= sizes[i];
  }
  return (row / stride) % sizes[which];
}

/// @brief Row @p Row of the product: one element from each container, by reference.
template <std::size_t Row, std::size_t... Which, class... Ts>
constexpr auto product_row(std::index_sequence<Which...>, const Ts &...containers) {
  using std::get;  // std containers by name, dual::array by ADL
  return std::tie(get<product_index<std::tuple_size_v<Ts>...>(Row, Which)>(containers)...);
}

/// @brief Every row of the product, in order.
///
/// The outer tuple's type is spelled out for the reason `zip` spells out its own: with a single
/// row, `std::tuple(x)` over a tuple `x` deduces a copy of `x` rather than a tuple holding it.
template <std::size_t... Rows, class... Ts>
constexpr auto product(std::index_sequence<Rows...>, const Ts &...containers) {
  using which = std::index_sequence_for<Ts...>;
  return std::tuple<decltype(product_row<Rows>(which{}, containers...))...>(
      product_row<Rows>(which{}, containers...)...);
}

}  // namespace details

/// @brief Every combination of one element from each of @p containers.
///
/// The result is a tuple with one row per combination, each row a tuple of references into the
/// containers, in row-major order: `product(xs, ys)` is `(x0, y0), (x0, y1), ..., (x1, y0), ...`.
/// It has the shape `zip` returns, so `summation` folds over it the same way:
///
///     summation(product(xs, ys), [](const auto &x, const auto &y) { return x * y; });
///
/// The rows refer into the containers, so the containers must outlive the result.
/// @tparam Ts Tuple-like container types: `std::array`, `std::tuple` or `dual::array`.
/// @param containers The containers to combine.
/// @return A tuple of `(size of each container multiplied together)` rows.
template <class... Ts>
constexpr auto product(const Ts &...containers) {
  constexpr std::size_t rows = (std::size_t{1} * ... * std::tuple_size_v<Ts>);
  return details::product(std::make_index_sequence<rows>{}, containers...);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_PRODUCT_HPP
