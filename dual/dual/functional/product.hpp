/// @file product.hpp
/// @brief Provides a utility to compute the Cartesian product of multiple
/// containers or views, returning a dual::array of tuples containing all combinations.

#pragma once

#include "dual/functional/view.hpp"

namespace dual {

/// @brief Computes the Cartesian product of multiple containers.
///        Each container is converted to a view, and the product is generated
///        as a dual::array of tuples.
/// @tparam Containers Types of the containers to combine.
/// @param cs References to the containers to be combined.
/// @return A dual::array containing tuples for each combination in the
/// Cartesian product.
template <class... Containers>
constexpr auto product(Containers &...cs) {
  return array(view(cs)...);
}

/// @brief Computes the Cartesian product of multiple constant containers.
///        Uses the storage of each container for the product.
/// @tparam Containers Types of the constant containers to combine.
/// @param cs Constant references to the containers to be combined.
/// @return A dual::array containing tuples for each combination in the
/// Cartesian product.
template <class... Containers>
constexpr auto product(const Containers &...cs) {
  return array(cs.storage()...);
}

}  // namespace dual
