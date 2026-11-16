/// ===============================================================================================
/// @file
///
/// @brief Provides utilities to apply a function to each element of a tuple or
/// dual::array. Uses dual::apply to invoke a function on each element.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_FOREACH_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_FOREACH_HPP

#include "uzu/foundation/dual/functional/apply.hpp"

namespace dual {

/// @brief Applies a function to each element of a std::tuple.
template <class F, class... Ts>
inline auto for_each(const std::tuple<Ts...> &data, F f) {
  apply([&](const auto &...elems) { (apply(f, elems), ...); }, data);
}

/// @brief Applies a function to each element of a dual::array.
template <class F, class... Ts>
inline auto for_each(const array<Ts...> &data, F f) {
  apply([&](const auto &...elems) { (apply(f, elems), ...); }, data.storage());
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_FOREACH_HPP
