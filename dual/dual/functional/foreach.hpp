/// @file foreach.hpp
/// @brief Provides utilities to apply a function to each element of a tuple or
/// dual::array. Uses dual::apply to invoke a processing function on each element.

#pragma once

#include "dual/functional/apply.hpp"

namespace dual {

/// @brief Applies a processing function to each element of a std::tuple.
/// @tparam F Type of the processing function or functor.
/// @tparam Ts Types of the tuple elements.
/// @param data The std::tuple to process.
/// @param process The function to apply to each element.
/// @return Result of applying the process function to each tuple element.
template <class F, class... Ts>
inline auto for_each(const std::tuple<Ts...> &data, F process) {
  apply([&](const auto &...elems) { (apply(process, elems), ...); }, data);
}

/// @brief Applies a processing function to each element of a dual::array.
/// @tparam F Type of the processing function or functor.
/// @tparam Ts Types of the array elements.
/// @param data The dual::array to process.
/// @param process The function to apply to each element.
/// @return Result of applying the process function to each array element.
template <class F, class... Ts>
inline auto for_each(const array<Ts...> &data, F process) {
  apply([&](const auto &...elems) { (apply(process, elems), ...); }, data.storage());
}

}  // namespace dual
