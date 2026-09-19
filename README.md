# uzu

A factor graph fitted with dual numbers, in header-only C++20.

Nodes hold estimates, edges score constraints, and the fit walks the whole
graph downhill using derivatives that come out of forward-mode dual numbers
rather than from a hand-written Jacobian. There is no Jacobian to write, and
no finite differences: the derivative of every residual with respect to every
node dimension falls out of evaluating the residual once.

```cpp
#include "uzu/graph.hpp"

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

auto a = point(init = {0.0, 0.0}, sigma = {0.4, 0.4});
auto b = point(init = {3.0, 1.0}, sigma = {0.4, 0.4});
auto e = between(sigma = {2.0, 2.0});

auto g = uzu::graph{
    uzu::gradient{lr = 0.05},
    uzu::nodes{uzu::key<1>(a), uzu::key<2>(b)},
    uzu::edges{uzu::link<1, 2>(e)}};

g.fit(iterations = 200);
```

## The two halves

| folder | what it is |
| --- | --- |
| [`dual/`](dual) | forward-mode dual numbers with flat, index-addressed derivative storage |
| [`optimizer/`](optimizer) | the factor graph: nodes, edges, the kernel, the algorithms |

`dual` is a vendored snapshot of
[`library-dual`](https://github.com/lcmonteiro/library-dual), carrying local
changes that are not upstream: the whole library is usable in a
constant expression, and it is built as C++20 with concepts in place of the
`enable_if` it shipped with. Its own README documents what was changed and what
each change measured.

`optimizer` is the part named `uzu`, and it is what the `uzu::` namespace holds.
`dual::` stays `dual::`, so a program using both says which half it is reaching
for.

## Building

Header-only, so there is nothing to build but the tests. Each half is its own
include root.

```sh
cd optimizer
g++ -std=c++20 -O2 -fconstexpr-ops-limit=100000000 -I. -I../dual test.cpp -o test && ./test

cd dual
g++ -std=c++20 -O2 -I. test.cpp -o test && ./test
```

Clang wants `-fconstexpr-steps=100000000` in place of `-fconstexpr-ops-limit`,
and is the same in every other respect. Both compilers are checked.

The optimizer's tests take about ten seconds, because **they run at compile
time**: every check is a `static_assert`, the fits are evaluated by the
compiler, and the test suite passing *is* the file compiling. That is also why
the constant-evaluation budget has to be raised.

## What is unusual here

- **No hand-written derivatives.** Forward-mode duals carry one index per node
  dimension through the whole residual, and the graph slices the result.
- **The tests are the compilation.** A failure names the property it broke, at
  the line that states it, before anything runs.
- **A redescending kernel.** Residuals go through `1 - (1 - z/N)^N`, which is
  Tukey's biweight generalised: past `sqrt(2N) * sigma` an outlier contributes
  nothing at all, and no derivative either. It is built from squarings, so it
  needs nothing from `<cmath>` and runs in a constant expression.
- **Keyword arguments.** `init`, `sigma`, `lr`, `beta` and `iterations` are
  named, so a node or an edge is built from what its values mean rather than
  from the order they are written in.
