/// ===============================================================================================
/// @file
///
/// @brief Provides utilities to create views of dual::array elements using
/// indices or directly.
/// ===============================================================================================
#ifndef UZU_FOUNDATION_DUAL_FUNCTIONAL_VIEW_HPP
#define UZU_FOUNDATION_DUAL_FUNCTIONAL_VIEW_HPP

#include "uzu/foundation/dual/array.hpp"

namespace dual {

/// @brief Creates a view of selected elements from a dual::array using
/// specified indices.
/// @tparam Is The indices of elements to include in the view.
/// @tparam Ts Types of the elements in the dual::array.
/// @param data The dual::array to view.
/// @return A dual::array containing references to the selected elements.
///
/// The index sequence is a tag: it is there to deduce `Is...` and is never read, so it is left
/// unnamed rather than named and ignored.
template <size_t... Is, class... Ts>
constexpr auto view(indices<Is...>, array<Ts...> &data) {
  return array(std::tie(std::get<Is>(data)...));
}

/// @brief Creates a view of all elements in a dual::array.
/// @tparam Ts Types of the elements in the dual::array.
/// @param data The dual::array to view.
/// @return A dual::array containing references to all elements.
template <class... Ts>
constexpr auto view(array<Ts...> &data) {
  return view(data.indices(), data);
}

}  // namespace dual

#endif  // UZU_FOUNDATION_DUAL_FUNCTIONAL_VIEW_HPP
