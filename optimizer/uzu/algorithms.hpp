/// @file algorithms.hpp
/// @brief What an algorithm is, and every algorithm there is.
///
/// The graph works out what the derivatives are and normalises each one against the sigma of the
/// dimension it belongs to; what to do with the result is a separate question, and this is where
/// the answers live. Splitting them out is what lets the update rule be stated at the point the
/// graph is built, next to the nodes and edges it applies to, instead of being fixed in the fit.
///
/// The kernel is deliberately *not* here. It reads a node's sigma, which is the graph's data, and
/// it is what makes a gradient comparable across dimensions of different scales - a property of
/// the problem, not of the method used to descend it. An algorithm sees only what comes out of it.
///
/// That is why this sits beside `graph/` rather than inside it, and why nothing under
/// `algorithms/` includes anything from `graph/`. The two share only the keyword vocabulary, which
/// is neither one's.
///
/// ## An algorithm builds a template instance
///
/// What the caller writes is not the thing that runs. An algorithm keeping one value per dual
/// index wants `std::array<value_type, Width>` for it, and `Width` is the graph's to know - but
/// the algorithm is written down before the graph it is handed to exists. So an algorithm is a
/// builder: it carries the hyper-parameters, names the instance a given width calls for, and
/// builds one on request.
///
/// ```cpp
/// template <std::size_t Width>
/// using instance = ...;                             // on the builder
///
/// template <std::size_t Width>
/// constexpr auto build() const -> instance<Width>;  // on the builder
///
/// template <std::size_t Index>
/// value_type step(value_type gradient);             // on the instance
/// ```
///
/// The split is invisible at the call site - `momentum{lr = 0.01, beta = 0.9}` is still what gets
/// written - and it buys four things over sizing the state at run time: the storage is an array
/// rather than a heap allocation, the index can be checked against the width at compile time
/// instead of being an unchecked subscript, no second call is needed to size the state, and a
/// builder holds no state, so one can be handed to two graphs without them treading on each
/// other.
///
/// A stateless algorithm is its own instance and builds a copy of itself.
///
/// The graph calls `step` once per index per pass, in index order, and holds the instance by
/// value. `step` is non-const so that an instance may keep state between calls.

#pragma once

#include "uzu/algorithms/gradient.hpp"
#include "uzu/algorithms/momentum.hpp"
