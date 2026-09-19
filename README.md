# uzu

uzu is a **header-only C++20 factor-graph optimizer** that gets its derivatives
from forward-mode dual numbers rather than from a hand-written Jacobian.

Nodes hold estimates, edges score constraints, and the fit walks the whole
graph downhill. There is no Jacobian to write and no finite differences: the
derivative of every residual with respect to every node dimension falls out of
evaluating the residual once.

> 💡 Write the residual once. uzu gives you the exact derivative automatically —
> and will evaluate the whole fit at compile time if you ask it to.

---

## Highlights

- **Write the residual once.** A derived edge implements a single templated
  `error(...)`. Evaluated with `double` it is the residual; evaluated with a
  dual number it carries its own exact partial derivatives.
- **Statically typed graph.** Node and edge types, their dimensions, and their
  connectivity are all part of the type system, resolved at compile time. A
  node's identity is a `key<N>`, so nothing is looked up at run time.
- **Allocation-free.** Every layout is a compile-time function of the node
  list, so the state is arrays and nothing is heap-allocated — which is also
  what lets a whole fit run inside a constant expression.
- **Usable in a constant expression.** The library, the duals included, is
  `constexpr` throughout. The test suite takes that literally: every assertion
  is a `static_assert`, so [the tests are the compilation](#testing).
- **A redescending kernel.** Residuals go through `1 - (1 - z/N)^N`, Tukey's
  biweight generalised: past `sqrt(2N) * sigma` an outlier contributes nothing
  at all, and no derivative either. Built from squarings, so it needs nothing
  from `<cmath>`.
- **Keyword arguments.** `init`, `sigma`, `lr`, `beta` and `iterations` are
  named, so a node or an edge is built from what its values mean rather than
  from the order they are written in.
- **No dependencies.** The standard library, and nothing else.

---

## Architecture

### Source layout

| Path | Responsibility |
| --- | --- |
| [include/uzu/foundation/dual/](include/uzu/foundation/dual/) | Dual-number types (`number`, `array`) and the math operations over them, for forward-mode automatic differentiation. |
| [include/uzu/foundation/types/](include/uzu/foundation/types/) | Supporting problem types built on the duals (the Bézier curve the benchmarks fit). |
| [include/uzu/helpers/](include/uzu/helpers/) | The keyword vocabulary — `init`, `sigma`, `lr`, `beta`, `iterations` — shared by the graph and the algorithms and owned by neither. |
| [include/uzu/optimization/](include/uzu/optimization/) | The optimizer: the `node` and `edge` bases, the `graph` that lays them out and fits them, the radial kernel, and the algorithms. |
| [tests/](tests/) | The compile-time suite. Every assertion is a `static_assert`, built against a scalar-generic 2D graph fixture. |
| [benchmarks/](benchmarks/) | Compile-time and run-time probes over the width of the derivative set. |
| [docs/](docs/) | [The optimizer in detail](docs/design.md), and the two documents on the vendored duals. |

> `include/uzu/foundation/` groups the core modeling modules (`dual`, `types`);
> `uzu/optimization/` is the layer that uses them.

`dual::` stays `dual::` and `uzu::` is the optimizer, so a program using both
says which half it is reaching for.

---

## How automatic differentiation works

Each derived edge implements **one** scalar-generic residual function:

```cpp
struct between : uzu::edge<between, std::array<double, 2>, 2> {
  using base = uzu::edge<between, std::array<double, 2>, 2>;
  using base::base;

  template <class A, class B>
  constexpr auto error(const A &a, const B &b) const {
    return (b - a) - measurement();
  }
};
```

- Evaluated with plain `double` → the **residual**, which is what the error is
  summed from.
- Evaluated with `dual::number` → the residual carries its **exact partial
  derivatives**. The graph seeds one dual index per node dimension, and reads
  each derivative straight back out of the result.

`plus` on a node and `error` on an edge are the whole of what a user writes.
Neither knows the graph exists, and `plus` is the only place that changes for
an estimate living on a manifold rather than in a vector space.

---

## Getting started

### Prerequisites

- A **C++20** compiler (CI builds with GCC and Clang).
- **CMake ≥ 3.24**.

Nothing is fetched, and there is nothing to link against.

### Build & test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Or in one step, prerequisites included: `./setup.sh`.

To use the headers without building the tests:

```bash
cmake -S . -B build -DUZU_BUILD_TESTS=OFF
```

### Use it in your project

The library exports the target `uzu::uzu`:

```cmake
add_subdirectory(uzu)          # or FetchContent
target_link_libraries(my_app PRIVATE uzu::uzu)
```

```cpp
#include "uzu.h"   // the node and edge bases, the graph, the algorithms
```

If your own fits are to be evaluated at compile time, ask the compiler for the
budget that needs — `-fconstexpr-ops-limit=100000000` on GCC,
`-fconstexpr-steps=100000000` on Clang. `uzu::uzu` deliberately does not
impose it: a consumer that only fits at run time does not need it. See
`uzu_constexpr_budget()` in [CMakeLists.txt](CMakeLists.txt) for the
per-compiler flag the tests use.

---

## Minimal example

Two points and a relative constraint between them. The fixture the tests are
built from is the same shape, and is the fuller version of this:
[tests/fixtures/simple_graph.hpp](tests/fixtures/simple_graph.hpp).

```cpp
#include <array>

#include "uzu.h"

struct point : uzu::node<point, std::array<double, 2>, 2> {
  using base = uzu::node<point, std::array<double, 2>, 2>;
  using base::base;

  template <class Delta>
  constexpr auto plus(const Delta &delta) const -> estimation_type {
    return {estimation()[0] + delta[0], estimation()[1] + delta[1]};
  }
};

struct between : uzu::edge<between, std::array<double, 2>, 2> {
  using base = uzu::edge<between, std::array<double, 2>, 2>;
  using base::base;

  template <class A, class B>
  constexpr auto error(const A &a, const B &b) const {
    return (b - a) - measurement();
  }
};

using uzu::edges, uzu::gradient, uzu::init, uzu::iterations;
using uzu::key, uzu::link, uzu::lr, uzu::nodes, uzu::sigma;

auto a = point(init = {0.0, 0.0}, sigma = {0.4, 0.4});
auto b = point(init = {3.0, 1.0}, sigma = {0.4, 0.4});
auto e = between(sigma = {2.0, 2.0});
e.measurement({2.0, 1.0});          // b - a was measured at (2, 1)

auto g = uzu::graph{
    gradient{lr = 0.05},
    nodes{key<1>(a), key<2>(b)},
    edges{link<1, 2>(e)}};

g.fit(iterations = 200);            // b - a is drawn towards (2, 1)
```

### Defining your own problem

1. **Node** — subclass `uzu::node<Derived, Estimation, Dimension>` and
   implement a scalar-generic `plus(delta)` manifold retraction.
2. **Edge** — subclass `uzu::edge<Derived, Measurement, Dimension>` and
   implement a scalar-generic `error(...)`.
3. **Graph** — `uzu::graph{algorithm, nodes{key<N>(...)...}, edges{link<N...>(...)...}}`,
   in that order: how to step, what to estimate, what constrains it.
4. Set estimations & measurements, call `fit(iterations = ...)`.

---

## Configuration

An algorithm is stated where the graph is built, next to the nodes and edges it
applies to, rather than being fixed inside the fit. Two ship with the library:

| Algorithm | Rule | Header |
| --- | --- | --- |
| `gradient{lr = ...}` | `-lr * g` | [graph_algorithm_gradient.hpp](include/uzu/optimization/graph_algorithm_gradient.hpp) |
| `momentum{lr = ..., beta = ...}` | the heavy ball: one velocity per dual index, accelerated by the bounded gradient and bled by `beta` | [graph_algorithm_momentum.hpp](include/uzu/optimization/graph_algorithm_momentum.hpp) |

What the caller writes is a *builder*: it carries the hyper-parameters, and the
graph asks it for an instance at the graph's own width. That is what lets the
per-index state be an array rather than an allocation, and one builder be
handed to two graphs without them treading on each other. See
[graph_algorithm.hpp](include/uzu/optimization/graph_algorithm.hpp) for what
writing another one takes.

The kernel is deliberately not the algorithm's: it reads a node's sigma, which
is the graph's data. An algorithm sees only the sign of the derivative and a
magnitude below one.

---

## Testing

```bash
ctest --test-dir build --output-on-failure
```

**Every assertion in `tests/` is a `static_assert`**, so each fit is evaluated
by the compiler and the suite passing *is* the suite compiling. A failed
property names itself at the line that states it, before anything runs. That
costs compile time and a raised evaluation budget; it buys a test that cannot
be built and then not run, and cannot pass on one machine and fail on another.
The executables `ctest` runs exist to confirm each translation unit was
actually built.

| Test | What it holds |
| --- | --- |
| [graph_derivatives_test.cpp](tests/graph_derivatives_test.cpp) | Every derivative against a central finite difference, per node and per dimension — the whole chain at once. |
| [graph_layout_test.cpp](tests/graph_layout_test.cpp) | One block per declared node however many edges name it, holding the sum of what reaches it; an unlinked node laid out and left alone. |
| [graph_descent_test.cpp](tests/graph_descent_test.cpp) | The error never rises across 200 passes, and an exact reflection symmetry survives the fit. |
| [graph_algorithm_test.cpp](tests/graph_algorithm_test.cpp) | `beta = 0` is plain descent exactly; the velocity accumulates as written; a builder carries no state; momentum converges in fewer passes. |
| [keywords_test.cpp](tests/keywords_test.cpp) | Sigma broadcasting, and keyword order independence. |
| [dual_test.cpp](tests/dual_test.cpp), [dual_array_test.cpp](tests/dual_array_test.cpp) | The duals themselves, as run-time programs carried over from the vendored snapshot. |

---

## What it is not

Gradient descent with a bounded step, not Gauss-Newton or Levenberg-Marquardt.
There is no linear solve, no information matrix, and no second-order
information at all — the point was to see how far the duals alone carry.
Convergence is correspondingly slow, and the saturating kernel means the fit
does not settle on the least-squares answer and should not be read as trying
to. [docs/design.md](docs/design.md) is explicit about both.

For the Gauss-Newton end of the same idea — Levenberg–Marquardt, Cholesky/PCG
back-ends, `std::pmr` arenas — see
[vortex](https://github.com/lcmonteiro/vortex), which shares this library's
ancestry and this repository's layout.

---

## Acknowledgements

- **[library-dual](https://github.com/lcmonteiro/library-dual)** — forward-mode
  automatic differentiation (dual numbers), vendored under
  `include/uzu/foundation/dual/`. See [docs/dual-snapshot.md](docs/dual-snapshot.md)
  for provenance and [docs/dual-storage.md](docs/dual-storage.md) for what has
  changed since.
- **[g2o](https://github.com/RainerKuemmerle/g2o)** — the graph-optimization
  shape this and [vortex](https://github.com/lcmonteiro/vortex) both follow.
