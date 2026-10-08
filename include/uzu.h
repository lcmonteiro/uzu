/// ===============================================================================================
/// @file
///
/// @brief uzu library
///
/// The whole optimizer in one include: the node and edge bases, the graph that lays them out, the
/// radial kernel, the algorithms, and the keyword vocabulary they are all built from.
/// ===============================================================================================
#ifndef UZU_H
#define UZU_H

// The graph names no algorithm, only the template parameter it was handed, so it does not
// include them -- and the algorithms include nothing of the graph. This umbrella is the one
// place that knows about both.
#include "uzu/optimization/algorithm.hpp"  // IWYU pragma: export
#include "uzu/optimization/graph.hpp"      // IWYU pragma: export

#endif  // UZU_H
