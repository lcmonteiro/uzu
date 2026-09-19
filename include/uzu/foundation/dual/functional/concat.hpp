/// ===============================================================================================
/// @file
///
/// @brief Provides utilities to concatenate multiple containers or views into a
/// single dual::array.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_CONCAT_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_CONCAT_HPP

#include "uzu/foundation/dual/functional/view.hpp"

namespace dual {

/// @brief Concatenates multiple containers by converting them to views and
/// joining their storages.
/// @tparam Containers Types of the containers to concatenate.
/// @param cs References to the containers to be concatenated.
/// @return A dual::array containing the concatenated elements.
template <class... Containers>
constexpr auto concat(Containers &...cs) {
  return array(std::tuple_cat(view(cs).storage()...));
}

/// @brief Concatenates multiple constant containers by joining their storages
/// directly.
/// @tparam Containers Types of the constant containers to concatenate.
/// @param cs Constant references to the containers to be concatenated.
/// @return A dual::array containing the concatenated elements.
template <class... Containers>
constexpr auto concat(const Containers &...cs) {
  return array(std::tuple_cat(cs.storage()...));
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_CONCAT_HPP
