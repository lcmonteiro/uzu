# uzu

**Factor-graph optimization at compile time.**

uzu is a header-only C++20 factor-graph optimizer where the optimization can be
done at compile time. Declare the problem in a `constexpr` function and the
compiler builds the graph, computes every derivative and runs every iteration
of the solver while it compiles your program. The optimization is finished
before the program exists; the binary carries only the result.

```cpp
constexpr auto fitted = solve();   // graph, derivatives, 200 iterations: all at compile time

static_assert(fitted.error < 1e-6, "verified at compile time, before the program exists");
```

The derivatives come from forward-mode dual numbers, so there is no Jacobian to
write and no finite differences: an edge states its residual once, and the
exact derivative with respect to every node dimension falls out of evaluating
it. The same code also runs at run time, unchanged - compile time is something
you ask for with `constexpr`, not a separate API.

> 💡 The optimization happens at compile time; the program only carries the
> answer. [See it done](#optimization-at-compile-time), and
> [the binary with no optimizer in it](#the-compiled-program-contains-no-optimizer).

---

## Optimization at compile time

Four corners of a square, each measured only *relative* to the next one, plus
one measurement saying where a single corner sits. No corner is told its own
position — the square has to come out of the four sides agreeing with each
other, and the anchor then places it.

```
c3 ______ c2          sides:  c1 - c0 = ( 2,  0)
  |      |                    c2 - c1 = ( 0,  2)
  |      |                    c3 - c2 = (-2,  0)
  |______|                    c0 - c3 = ( 0, -2)
c0        c1          anchor: c0      = ( 1,  1)
```

Every corner starts at the origin, on top of every other, so the fit has to
separate them as well as place them. Here is the whole of it —
[examples/compile_time_fit.cpp](examples/compile_time_fit.cpp) is this, with
the node and edge types written out:

```cpp
constexpr auto solve() -> square {
  auto c0 = corner(init = {0.0, 0.0}, sigma = {0.005, 0.005});
  /* c1, c2, c3 the same */

  auto bottom  = side  (init = { 2.0,  0.0}, sigma = {4.0, 4.0});
  auto right   = side  (init = { 0.0,  2.0}, sigma = {4.0, 4.0});
  auto top     = side  (init = {-2.0,  0.0}, sigma = {4.0, 4.0});
  auto closing = side  (init = { 0.0, -2.0}, sigma = {4.0, 4.0});
  auto at      = anchor(init = { 1.0,  1.0}, sigma = {4.0, 4.0});

  auto graph = uzu::graph{
      momentum{lr = 0.05, beta = 0.9},
      nodes{
          key<0>(c0),
          key<1>(c1),
          key<2>(c2),
          key<3>(c3)},
      edges{
          link<0, 1>(bottom),
          link<1, 2>(right),
          link<2, 3>(top),
          link<3, 0>(closing),
          link<0>(at)}};

  graph.fit(iterations = 200);
  return {{c0.estimation(), c1.estimation(), c2.estimation(), c3.estimation()},
          graph.error()};
}

// The line that matters. `constexpr` is not decoration here: it says the
// initializer must be a constant expression, so the compiler has to run the
// whole fit — build the graph, seed the duals, take two hundred passes of
// gradient descent with momentum — and fail to compile if it cannot.
constexpr auto fitted = solve();
```

Which means the answer can be checked while the program is still being
compiled. A failure here is not a red test, it is a build that does not finish:

```cpp
static_assert(at_corner<0>(1.0, 1.0), "the anchored corner sits where it was measured");
static_assert(at_corner<1>(3.0, 1.0), "two units along x from it");
static_assert(at_corner<2>(3.0, 3.0), "and two up, so the sides are square");
static_assert(at_corner<3>(1.0, 3.0), "and the fourth closes the loop");
static_assert(fitted.error < 1e-6,    "all five measurements are satisfied at once");

// And because it is a constant, it can go where only a constant can.
using side_length = std::integral_constant<int, /* corner 1 minus corner 0 */>;
static_assert(side_length::value == 2, "the square the fit found is two units on a side");
```

```
$ cmake --build build --target uzu_compile_time_fit   # about 3 s on GCC
$ ./build/examples/uzu_compile_time_fit

a 2 x 2 square, solved before this program started running:

  corner 0 = (1.0014, 1.0014)
  corner 1 = (3.0019, 1.0020)
  corner 2 = (3.0021, 3.0021)
  corner 3 = (1.0020, 3.0019)

  error = 1.64e-07
```

### The compiled program contains no optimizer

That is the part worth checking rather than taking on faith. Build the same
example twice — once as written, once with the anchor read from `argv` so
nothing can be folded away — and compare what lands in the binary:

| the fit runs at | `.text` | functions emitted |
| --- | ---: | ---: |
| **compile time** | **377 B** | **11** |
| run time | 5290 B | 16 |

`main` in the compile-time build is three `printf` calls and a loop over four
pairs of doubles. There is no graph in it, no dual number, and no descent —
those are all in the compiler's memory, and none of them survived into the
program. The answer did. `objdump -s -j .rodata` on the compile-time build dumps the
read-only data, and decoding the bytes of the `fitted` object that `main`
reads from gives:

```
1.0014  1.0014    3.0019  1.0020    3.0021  3.0021    1.0020  3.0019
```

Eight doubles, which are the four corners, sitting in the executable as data.
The optimization is not fast in this program — it already happened.

> GCC needs no flag for this: its default constant-evaluation budget is about
> thirty million operations and the fit uses fewer. Clang's default is about a
> million, so it wants `-fconstexpr-steps=10000000`. `CMakeLists.txt` asks for
> whichever the compiler in use spells, so `cmake --build` is all that is
> needed either way.

### Writing a compile-time optimization

Any fit can be moved to compile time; these are the rules it has to follow.

1. **Put the whole problem in one `constexpr` function.** Declare the nodes,
   the edges and the graph as locals, fit, and return plain values - the
   estimates, the error. The graph points at its nodes and edges, so it cannot
   outlive them, and a `constexpr` variable can only hold the values that come
   out.
2. **Make `plus` and `error` `constexpr`,** and keep them to `+`, `-`, `*`, `/`
   and negation. Those are constant expressions on every compiler. `sqrt`,
   `sin`, `cos`, `exp`, `log` and `pow` go through `<cmath>`, which is not
   `constexpr` before C++26: GCC accepts them anyway as an extension, Clang
   refuses. The kernel is built from squarings for exactly this reason.
3. **No heap, no I/O, no randomness.** uzu needs none of them - every layout is
   an array sized at compile time - but the problem's data has to be fixed in
   the source, too.
4. **Give the compiler the budget.** Constant evaluation is capped by a step
   count, and a fit's cost grows roughly with passes × edges × width. The
   square above fits GCC's default; Clang's default is about a million steps
   and wants `-fconstexpr-steps=10000000`. The tests ask for `10^8`, and
   `uzu_constexpr_budget()` in [CMakeLists.txt](CMakeLists.txt) spells it for
   each compiler. Running past it is a compile error that says so.
5. **Check the answer with `static_assert`.** A wrong answer is then a build
   that does not finish, at the line that states the property.

Everything else carries over: both algorithms and
[switching nodes and edges off](#enabling-and-disabling-nodes-and-edges) work
inside a constant expression - the tests exercise each of them there. Drop
`constexpr` from the variable and the same function runs when the program
does.

---

## Highlights

- **Optimization at compile time.** The library, the duals included, is
  `constexpr` throughout, so a whole fit - graph, derivatives, every
  iteration - can run at compile time, its answer a compile-time constant,
  and the optimizer [nowhere in the resulting binary](#the-compiled-program-contains-no-optimizer).
  The test suite takes the same idea literally: every assertion is a
  `static_assert`, so [the tests are the compilation](#testing).
- **Write the residual once.** A derived edge implements a single templated
  `error(...)`. Evaluated with `double` it is the residual; evaluated with a
  dual number it carries its own exact partial derivatives.
- **Statically typed graph.** Node and edge types, their dimensions, and their
  connectivity are all part of the type system, resolved at compile time. A
  node's identity is a `key<N>`, so nothing is looked up at run time.
- **Allocation-free.** Every layout is a compile-time function of the node
  list, so the state is arrays and nothing is heap-allocated — the condition
  for a fit to run inside a constant expression at all.
- **A redescending kernel.** Residuals go through `1 - (1 - z/N)^N`, Tukey's
  biweight generalised: past `sqrt(2N) * sigma` an outlier contributes nothing
  at all, and no derivative either. Built from squarings, so it needs nothing
  from `<cmath>` and stays a constant expression on every compiler.
- **Keyword arguments.** `init`, `sigma`, `lr`, `beta` and `iterations` are
  named, so a node or an edge is built from what its values mean rather than
  from the order they are written in. A node takes `init` and `sigma`; an edge
  takes the same two, where `init` is the measurement.
- **No dependencies.** The standard library, and nothing else.

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
[Writing a compile-time optimization](#writing-a-compile-time-optimization).

---

## Minimal example

Two points and a relative constraint between them, fitted by the compiler. The
fixture the tests are built from is the same shape, and is the fuller version
of this: [tests/fixtures/simple_graph.hpp](tests/fixtures/simple_graph.hpp).

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

using uzu::beta, uzu::edges, uzu::init, uzu::iterations, uzu::momentum;
using uzu::key, uzu::link, uzu::lr, uzu::nodes, uzu::sigma;

constexpr auto solve() -> std::array<double, 2> {
  auto a = point(init = {0.0, 0.0}, sigma = {0.4, 0.4});
  auto b = point(init = {3.0, 1.0}, sigma = {0.4, 0.4});
  auto e = between(init = {2.0, 1.0}, sigma = {2.0, 2.0});   // b - a was measured at (2, 1)

  auto g = uzu::graph{
      momentum{lr = 0.05, beta = 0.9},
      nodes{key<1>(a), key<2>(b)},
      edges{link<1, 2>(e)}};

  g.fit(iterations = 200);
  return {b.estimation()[0] - a.estimation()[0], b.estimation()[1] - a.estimation()[1]};
}

constexpr auto offset = solve();   // the fit, run by the compiler: (1.978, 1.000)

constexpr auto near(double x, double y) { return (x - y) * (x - y) < 0.05 * 0.05; }
static_assert(near(offset[0], 2.0) && near(offset[1], 1.0), "b - a was drawn to (2, 1)");
```

### Defining your own problem

1. **Node** — subclass `uzu::node<Derived, Estimation, Dimension>` and
   implement a scalar-generic `plus(delta)` manifold retraction. Build it with
   `init` (the starting estimate) and `sigma`.
2. **Edge** — subclass `uzu::edge<Derived, Measurement, Dimension>` and
   implement a scalar-generic `error(...)`. Build it with `init` (the
   measurement) and `sigma`, or set the measurement later with
   `measurement(...)` when it arrives from somewhere else.
3. **Graph** — `uzu::graph{algorithm, nodes{key<N>(...)...}, edges{link<N...>(...)...}}`,
   in that order: how to step, what to estimate, what constrains it.
4. Set estimations & measurements, call `fit(iterations = ...)` - inside a
   `constexpr` function to have the compiler run it, or anywhere to run it
   with the program.

---

## Configuration

An algorithm is stated where the graph is built, next to the nodes and edges it
applies to, rather than being fixed inside the fit. Two ship with the library:

| Algorithm | Rule | Header |
| --- | --- | --- |
| `gradient{lr = ...}` | `-lr * g` | [algorithm/gradient.hpp](include/uzu/optimization/algorithm/gradient.hpp) |
| `momentum{lr = ..., beta = ...}` | the heavy ball: one velocity per dual index, accelerated by the bounded gradient and bled by `beta` | [algorithm/momentum.hpp](include/uzu/optimization/algorithm/momentum.hpp) |

What the caller writes is a *builder*: it carries the hyper-parameters, and the
graph asks it for an instance at the graph's own width. That is what lets the
per-index state be an array rather than an allocation, and one builder be
handed to two graphs without them treading on each other. See
[algorithm.hpp](include/uzu/optimization/algorithm.hpp) for what
writing another one takes.

The kernel is deliberately not the algorithm's: it reads a node's sigma, which
is the graph's data. An algorithm sees only the sign of the derivative and a
magnitude below one.

### Enabling and disabling nodes and edges

The graph's shape is fixed at compile time, but any node or edge can be
switched off at run time, and back on, between fits:

```cpp
loop_closure.disable();  // an edge: left out of error(), gradient() and fit()
anchor.disable();        // a node: held where it is; edges still read it
graph.fit(iterations = 100);

anchor.enable();         // both switch back on
loop_closure.enable();
```

A disabled edge is not evaluated at all, so the graph behaves exactly as if it
had been declared without it. A disabled node is a fixed one: `fit` leaves its
estimate alone and does not call the algorithm for its indices, while the edges
around it keep reading it as a constant. Everything starts enabled.

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
| [graph_enable_test.cpp](tests/graph_enable_test.cpp) | A disabled edge leaves the error and gradient exactly as if it were not declared; a disabled node is held while its neighbour moves; both switch back on. |
| [graph_contracts_test.cpp](tests/graph_contracts_test.cpp) | The edge contract on its own: an edge over an undeclared key is caught, and an empty edge list passes. |
| [keyed_layout_test.cpp](tests/keyed_layout_test.cpp) | The generic layout: lookups, offsets, width, and a repeated key caught, with no graph involved. |
| [dual_derivatives_test.cpp](tests/dual_derivatives_test.cpp) | `dual::zero` widens a sum to every index; `dual::derivatives` reads a full range or a slice back out. |
| [keywords_test.cpp](tests/keywords_test.cpp) | Sigma broadcasting, and keyword order independence. |
| [trajectory_compile_time_test.cpp](tests/trajectory_compile_time_test.cpp) | A four-pose chain, started from one point, recovered exactly — by the compiler. |
| [optimization_trajectory_test.cpp](tests/optimization_trajectory_test.cpp) | A noisy 12- and 24-pose trajectory with random initial guesses and loop closures, recovered to within vortex's own accuracy bound. The one test here that runs rather than compiles, and most of the suite's build time. |
| [dual_test.cpp](tests/dual_test.cpp), [dual_array_test.cpp](tests/dual_array_test.cpp) | The duals themselves, as run-time programs carried over from the vendored snapshot. |

A clean build of the whole suite takes about 47 s on GCC and 59 s on Clang
with `-j`, most of it the trajectory pair. Every test is deterministic: the
randomized problem draws from `mt19937` directly rather than through
`std::uniform_real_distribution`, which is not specified to give the same
sequence across standard libraries, so both compilers print the same numbers.

---

## Architecture

### Source layout

| Path | Responsibility |
| --- | --- |
| [include/uzu/foundation/dual/](include/uzu/foundation/dual/) | Dual-number types (`number`, `array`) and the math operations over them, for forward-mode automatic differentiation. |
| [include/uzu/foundation/types/](include/uzu/foundation/types/) | Supporting problem types built on the duals (the Bézier curve the benchmarks fit). |
| [include/uzu/foundation/meta/](include/uzu/foundation/meta/) | Generic compile-time machinery: the keyed index layout the graph lays its nodes out with. |
| [include/uzu/helpers/](include/uzu/helpers/) | The keyword vocabulary — `init`, `sigma`, `lr`, `beta`, `iterations` — shared by the graph and the algorithms and owned by neither. |
| [include/uzu/optimization/](include/uzu/optimization/) | The optimizer: under `graph/`, the `node` and `edge` bases and the `graph` that fits them; beside it, the radial kernel and the algorithms. |
| [tests/](tests/) | The compile-time suite. Every assertion is a `static_assert`, built against a scalar-generic 2D graph fixture. |
| [examples/](examples/) | A square recovered from its sides, fitted by the compiler — see [below](#optimization-at-compile-time). |
| [benchmarks/](benchmarks/) | Compile-time and run-time probes over the width of the derivative set. |
| [docs/](docs/) | [The optimizer in detail](docs/design.md), and the two documents on the vendored duals. |

> `include/uzu/foundation/` groups the core modeling modules (`dual`, `types`);
> `uzu/optimization/` is the layer that uses them.

`dual::` stays `dual::` and `uzu::` is the optimizer, so a program using both
says which half it is reaching for.

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

[optimization_trajectory_test.cpp](tests/optimization_trajectory_test.cpp) runs
vortex's own trajectory problem here, and is the most direct measurement of what
that difference costs: uzu recovers the reference to within vortex's accuracy
bound, but at 12 and 24 poses rather than 100 and 600, and only after being told
two sigmas and a step size that vortex's solver works out for itself. Its
comments carry the measured numbers.

---

## Acknowledgements

- **[library-dual](https://github.com/lcmonteiro/library-dual)** — forward-mode
  automatic differentiation (dual numbers), vendored under
  `include/uzu/foundation/dual/`. See [docs/dual-snapshot.md](docs/dual-snapshot.md)
  for provenance and [docs/dual-storage.md](docs/dual-storage.md) for what has
  changed since.
- **[g2o](https://github.com/RainerKuemmerle/g2o)** — the graph-optimization
  shape this and [vortex](https://github.com/lcmonteiro/vortex) both follow.
