/// @file zip.hpp
/// @brief Provides utilities to zip multiple containers or tuples together,
/// element-wise, producing tuples of corresponding elements from each container.

#pragma once

#include <algorithm>
#include <tuple>

#include "dual/helpers/indices.hpp"

namespace dual {

/// @brief Internal details for zipping containers.
namespace details {
/// @brief Zips the I-th element from each container into a tuple.
/// @tparam I Index of the element to zip.
/// @tparam Ts Types of the containers.
/// @param containers The containers to zip.
/// @return A tuple containing the I-th element from each container.
template <size_t I, class... Ts>
constexpr auto zip(const Ts &...containers) {
  return std::tie(std::get<I>(containers)...);
}
}  // namespace details

/// @brief Zips the elements at specified indices from each container into
/// tuples.
/// @tparam Is Indices to zip.
/// @tparam Ts Types of the containers.
/// @param idx Indices to select.
/// @param containers The containers to zip.
/// @return A tuple containing tuples of the selected elements from each
/// container.
template <size_t... Is, class... Ts>
constexpr auto zip(indices<Is...> idx, const Ts &...containers) {
  return std::tuple(details::zip<Is>(containers...)...);
}

/// @brief Zips multiple containers together, element-wise.
///        The result contains tuples where each tuple holds the elements from
///        each container at the same index. The number of tuples is determined
///        by the smallest container size.
/// @tparam Ts Types of the containers.
/// @param containers The containers to zip.
/// @return A tuple containing tuples of corresponding elements from each
/// container.
template <class... Ts>
constexpr auto zip(const Ts &...containers) {
  constexpr auto N = std::min({std::tuple_size_v<Ts>...});
  return zip(indices_sequence_t<0, N>{}, containers...);
}

}  // namespace dual
