/// ===============================================================================================
/// @file
///
/// @brief The factor graph: the `node` and `edge` bases, how they enter a graph, and the graph
/// that lays them out and fits them.
///
/// Everything under `graph/`, in one include - the way `uzu/foundation/dual.hpp` gathers
/// `dual/`. The algorithms are not included: the graph names none, only the template parameter it
/// is handed. See `uzu/optimization/algorithm.hpp` for those, or `uzu.h` for both.
/// ===============================================================================================
#ifndef UZU_OPTIMIZATION_GRAPH_HPP
#define UZU_OPTIMIZATION_GRAPH_HPP

#include "uzu/optimization/graph/contracts.hpp"  // IWYU pragma: export
#include "uzu/optimization/graph/edge.hpp"       // IWYU pragma: export
#include "uzu/optimization/graph/entries.hpp"    // IWYU pragma: export
#include "uzu/optimization/graph/graph.hpp"      // IWYU pragma: export
#include "uzu/optimization/graph/node.hpp"       // IWYU pragma: export

#endif  // UZU_OPTIMIZATION_GRAPH_HPP
