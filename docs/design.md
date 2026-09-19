# The optimizer, in detail

How the graph is put together and why it is put together that way. The
[README](../README.md) is the short version; this is the reasoning behind it.

A small graph optimizer built on the
[vendored dual numbers](../include/uzu/foundation/dual). Nodes hold estimates,
edges score constraints, and the fit walks the whole graph downhill using
derivatives that come out of the forward-mode duals rather than from a
hand-written Jacobian. It is header-only and **C++20**, the standard the
duals share.

```
include/
  uzu.h                                 the lot, in one include
  uzu/helpers/
    keywords.hpp                        init, sigma, lr, beta, iterations
  uzu/optimization/
    graph.hpp                           the layout and the fit
    graph_node.hpp                      the node base
    graph_edge.hpp                      the edge base
    graph_kernel.hpp                    the radial kernel
    graph_algorithm.hpp                 what an algorithm has to supply
    graph_algorithm_gradient.hpp        plain descent
    graph_algorithm_momentum.hpp        the heavy ball
  uzu/foundation/dual/                  the dual numbers
```

The graph headers and the algorithm headers do not include each other. An
algorithm is not the graph's - it never sees a node, an edge, or the kernel,
only a gradient the graph has already normalised - and the graph names no
algorithm, only the template parameter it was handed. They share the keyword
vocabulary, which is neither one's, and that is why it sits in
`uzu/helpers/` rather than in either.

The tests take about ten seconds to build, because **they run at compile
time** - see below - which is why the compiler needs a raised budget for
constant evaluation. `CMakeLists.txt` asks for it per compiler:
`-fconstexpr-ops-limit=100000000` on GCC, `-fconstexpr-steps=100000000` on
Clang.

What a graph looks like, written out:

## The shape of it

```cpp
struct node_a : uzu::node<node_a, std::array<double, 2>, 2> {
  using base = uzu::node<node_a, std::array<double, 2>, 2>;
  using base::base;

  template <class Delta>
  auto plus(const Delta &delta) const -> estimation_type {
    auto out = estimation();
    for (std::size_t i = 0; i < out.size(); ++i) {
      out[i] += delta[i];
    }
    return out;
  }
};

struct edge_a : uzu::edge<edge_a, std::array<double, 0>, 2> {
  using base = uzu::edge<edge_a, std::array<double, 0>, 2>;
  using base::base;

  template <class A, class B>
  auto error(const A &a, const B &b) const {
    return b - a;
  }
};

struct edge_b : uzu::edge<edge_b, std::array<double, 2>, 2> {
  using base = uzu::edge<edge_b, std::array<double, 2>, 2>;
  using base::base;

  template <class B>
  auto error(const B &b) const {
    return b - measurement();
  }
};

auto n1 = node_a(init = {1.0, 1.0}, sigma = {0.2, 0.2});
auto n2 = node_a(init = {1.0, 1.0}, sigma = {0.2, 0.2});

auto e1 = edge_a(sigma = {2.0, 3.0});
auto e2 = edge_b(sigma = {2.0});
auto e3 = edge_b(sigma = {2.0});

e2.measurement({0.0, 0.0});
e3.measurement({4.0, 2.0});

auto g = uzu::graph{
    uzu::gradient{lr = 0.05},
    uzu::nodes{
        uzu::key<1>(n1),
        uzu::key<2>(n2),
    },
    uzu::edges{
        uzu::link<1, 2>(e1),
        uzu::link<1>(e2),
        uzu::link<2>(e3),
    },
};

g.fit(iterations = 4000);

std::cout << n1.estimation()[0];
```

A node supplies `plus`, an edge supplies `error`, and neither knows the graph
exists. `plus` is the only place that needs to change for an estimate that
lives on a manifold rather than in a vector space.

### What a node and an edge are declared with

```cpp
template <class Derived, class Estimation, auto Dimension>  class node;
template <class Derived, class Measurement, auto Dimension> class edge;
```

Both take what they hold as a type rather than a scalar and a count, and name it
for what it is: a node's `Estimation` is `estimation_type`, what `estimation()`
gives back and `plus` returns; an edge's `Measurement` is `measurement_type`,
what it is scored against. Each supplies the scalar as its own `value_type`, and
that is what the base then calls `value_type` too - so `value_type` keeps the
meaning it has everywhere else, and the thing being held has a name of its
own.

`Dimension` is the width of the vector that crosses the boundary in each case -
what `plus` receives for a node, what `error` returns for an edge. Declaring the
edge's rather than deducing it from what `error` happens to return buys a check:
the two are compared, and an edge that returns the wrong number of residuals is
a compile error naming the edge, not a sigma quietly zipped against a row of a
different length.

Sigma is one per `Dimension`, so `sigma = {2.0, 3.0}` sets them apart and
`sigma = {2.0}` is broadcast across them all. That moved into the keyword, which
is what let the old `Sigmas` template parameter go: an edge whose dimensions are
alike is written the short way without declaring anything about it.

A node's estimate and its delta must, for now, be the same width - there is a
`static_assert` that says so and why. Seeding takes one dual index per estimate
coordinate while the layout hands out one per `Dimension`, so a wider estimate
would run past that node's block and alias the next one's. Supporting the case
the two names invite - a unit quaternion, four numbers with three directions -
means seeding a zero delta and pushing it through `plus` instead, which would
make `plus` generic over duals rather than a function returning a concrete
estimate. That is a change to what every node author writes, and it has not been
made.

A graph is three things in a fixed order: how to step, what to estimate, and
what constrains it.

### The algorithm

`gradient{lr = 0.05}` is the update rule, stated where the graph is
built rather than fixed inside the fit. What the caller writes, though, is not
the thing that runs: **an algorithm is a builder of template instances.**

```cpp
template <std::size_t Width>
using instance = ...;                              // on the builder

template <std::size_t Width>
constexpr auto build() const -> instance<Width>;   // on the builder

template <std::size_t Index>
auto step(value_type gradient) -> value_type;      // on the instance
```

An algorithm that keeps one value per dual index wants that state to be a
`std::array<value_type, Width>` - but `Width` is the graph's to know, and the
algorithm is written down before the graph it is handed to exists. So the
builder carries the hyper-parameters, names the instance a given width calls
for, and builds one on request. A stateless algorithm is its own instance and
builds a copy of itself.

The split is invisible at the call site: `momentum{lr = 0.01, beta = 0.9}` is
still all that gets written. What it buys over sizing the state at run time is
that the storage is an array rather than a heap allocation, that an index can be
checked against the width at compile time rather than being an unchecked
subscript, that there is no second call needed to size the state - and that a
builder holds no state, so one can be handed to two graphs without them treading
on each other.

The index is a template argument because that is what it is: the layout is a
compile-time function of the node list, so an algorithm can dispatch on an index
as readily as it can store against one. The graph calls `step` in index order
and holds the instance by value; `step` is non-const so that state may be
carried between calls. Since the algorithm owns the step size, `fit` says
only how many passes to take.

**The kernel is not the algorithm's.** It reads a node's sigma, which is the
graph's data, and what it does - make a gradient comparable across dimensions
of different scales, and bound it - belongs to the problem rather than to the
method used to descend it. The graph applies it before calling `step`, so what
an algorithm sees is the sign of the derivative and a magnitude below one,
whichever dimension it came from. `gradient` is then the whole of the
plain rule:

```cpp
template <std::size_t Index>
auto step(value_type gradient) -> value_type {
  return -rate_ * gradient;
}
```

`momentum` is the heavy ball: each index carries a velocity - one array slot,
no allocation - that the bounded gradient accelerates and `beta` bleeds away.
It lives in `uzu/optimization/graph_algorithm_momentum.hpp`, one header per algorithm, each
of which compiles on its own.

```cpp
auto algorithm = uzu::momentum{lr = 0.02, beta = 0.9};

//  v     <- beta * v + g
//  delta  = -lr * v
```

Along a direction the gradient keeps pointing the same way, the velocity settles
at `g / (1 - beta)`, so the step there grows to `1 / (1 - beta)` times the plain
one. That is the point, and also the catch: the kernel bounds the *gradient* to
one, not the step, so at `beta = 0.9` a step can reach ten times `lr`. Momentum
trades the plain rule's hard bound for speed along a consistent slope, and a
momentum `lr` is not comparable to a descent `lr` of the same number.

On the single-node pull the tests use, at one `lr`, the difference is most of
an order of magnitude - the test prints the two counts as it runs:

```
(passes to error < 0.02: descent 1668, momentum 145)
```

`beta = 0` leaves `v = g`, and the rule is then exactly plain descent - which is
what the tests hold it to, to the bit rather than to a tolerance.

### Nodes and edges

The two lists are named for what they hold, and the entries for what declaring
one does: `nodes{key<1>(n1)}` keys a node, `edges{link<1, 2>(e1)}` links an
edge over the keys it constrains. `node` and `edge` are also the bases the two
derive from, so the lists read as the plurals of the things in them, and
`link_entry` stays the name of what `link` produces - a record of one edge and
the keys it was linked over.

They are two lists rather than one because they are two kinds of thing. Every
place the graph used to ask an entry which of the two it was is a place it now
does not have to: the index layout walks the node list, the error walks the edge
list, and neither skips over the other. The gain is not in line count - the
header is longer than the single-list version it replaced - but in what can be
checked, below.

### Named arguments

`init = {1.0, 1.0}` is a real keyword argument, not a comment. `init` is an
object whose `operator=` takes `const T (&)[N]` - a braced list will not deduce
a `std::array`, but it binds to a reference to a raw array and carries its
length along. The constructors then pick their arguments out by tag, so order
does not matter and a missing keyword falls back to a documented default.

A keyword takes the type of that default rather than the one written at the
call site, so `lr = 1` means `lr = 1.0`. Without that, the integer would reach
a `double` member through a braced initialiser, which is a narrowing conversion
and diagnosed as one.

### Operations an edge reaches for

`uzu.h` includes only the operations the fit itself performs -
`plus`, `minus`, and `multiplies`. An edge whose `error` reaches for anything
else has to include it by name:

```cpp
#include "uzu/foundation/dual/operations/cos.hpp"
#include "uzu/foundation/dual/operations/sin.hpp"
```

Leaving them out does not fail at the call site. `number` converts implicitly
to `const double &`, so `std::sin` resolves to the ordinary one, takes the
value, and hands back a plain `double` with no derivative attached. An edge
collecting its residual into a `dual::array` is caught, because that
`static_assert` rejects a `double` element - but one that folded its residual
down to a scalar would have no such guard, and the fit would run against a
derivative that was silently always zero.

## What the fit does, per pass

1. **Seed.** Every node's estimate becomes a `dual::array`, one dual index per
   dimension.
2. **Score.** Each edge's `error` is called with the estimates of the nodes its
   it links, and returns one residual per error dimension.
3. **Robustify.** Each residual goes through `1 - (1 - z/N)^N`, with
   `z = e^2 / 2s^2` and that dimension's sigma. Near zero this is `z`, so a
   small residual is penalised exactly as least squares would penalise it. It
   saturates at 1, so an outlier contributes a bounded amount and, more
   usefully, a vanishing derivative - it stops pulling. Past
   `sqrt(2N) * sigma` it stops pulling entirely - see below.
4. **Accumulate across dimensions**, then **across edges**. What comes out is a
   single dual number: one value, and one derivative per index in the graph.
5. **Split.** Each node takes the slice of those derivatives that belongs to it.
6. **Bound.** Each gradient component goes through the same kernel shape with
   the *node's* sigma, and is given its sign back, so what comes out is the
   direction of the derivative with a magnitude below one, on a scale that does
   not depend on which dimension it came from.
7. **Step.** The algorithm turns that into a delta. `gradient` scales
   it by `lr`; `momentum` accumulates it into that index's velocity first.
8. **Move.** `plus` is called with the resulting delta.

The kernel is even, so it is the magnitude that goes through it and the sign
that is restored afterwards. Skipping that would make the step climb for every
negative gradient - the fit would still look like it was doing something, and
it would be wrong.

Steps 1 to 6 are the graph's. Step 7 is the algorithm's, and step 8 the node's.

## The index layout

Dual indices are compile-time template arguments, so the layout has to be a
compile-time function of the entry list. Keys are what make that possible: a
node's identity is a constant, so it gets **one** block of indices however many
edges name it, and each edge finds its nodes by looking their keys up. Nothing
is matched at run time.

Nodes take their blocks in declaration order, so the width of a graph is the
sum of its declared nodes' dimensions and nothing else. A node no edge names
still takes its block, and simply does not move. An edge linked to a key no node
declares is a compile error, and so are two nodes declared under the same key -
the lookups take the first match, so a repeated key would otherwise be silent
and wrong: both nodes would read one gradient and step identically, while the
second one's block sat in the layout with nothing referring to it.

Both fire where the graph is declared, and report once. That is what separating
the two lists buys: with every edge in hand at the class, an undeclared key can
be caught there rather than where an edge's nodes are looked up. It used to be
checked in that lookup, which meant a graph with a mistyped key compiled
silently until something instantiated the fit, and then reported it behind two
further errors about tuple indices being out of range.

An earlier version of this had no keys: edges held node references directly,
each edge occupied its own block, and a node named by two edges got two blocks
that were matched by address at fit time and summed before stepping. That is
correct - the chain rule says the node's gradient is that sum - but it costs a
block per *occurrence* rather than per node, and every distinct index set is a
distinct `number` type, which is what drives compile time in this library.

On a chain of N nodes with N-1 relative edges and N absolute ones, that is
`6N-4` indices against `2N`, and it shows:

| N | indices, addresses | indices, keys | compile, addresses | compile, keys | |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 3 | 14 |  6 | 1.21 s | 1.13 s | 0.94x |
| 4 | 20 |  8 | 1.63 s | 1.44 s | 0.88x |
| 5 | 26 | 10 | 2.41 s | 1.86 s | 0.77x |
| 6 | 32 | 12 | 3.57 s | 2.25 s | 0.63x |

The two agree to the last digit of the error at every N. The keys buy the
layout, not a different answer.

The one place keys lose is a graph small enough that the index saving does not
pay for the headers: on a two-node graph the keyed version measured 0.88 s
against 0.64 s, because it reaches for `zip` and `summation` where the other
hand-rolled the fold. A three-node chain already pays that back.

## What is checked

Every check is a `static_assert`, so the fits below are run by the compiler and
this file passing its tests *is* this file compiling. Each scenario is a
`constexpr` function returning what it measured, each result is a `constexpr`
variable, and each property is asserted against it by name - so a failure names
the property at the line that states it, rather than printing at run time.

```cpp
constexpr auto derivatives = measure_derivatives();

static_assert(derivatives.width == 4, "one block per declared node, ...");
static_assert(derivatives.worst <= 1e-6, "every derivative matches a central difference");
```

That needs the whole chain to be `constexpr`, and `<cmath>` is not - none of it
is, before C++26 - so the kernel cannot call `std::exp`. It does not need to.
The kernel is `kernel::order` squarings of a dual number and nothing else: no
series, no range reduction, and nothing from `<cmath>` standing between the fit
and the compiler. The duals' operations, functional layer and `array` are marked
`constexpr`, and the optimizer's own headers are too.

### Picking the kernel

`N = 2^kernel::order` is the one number that picks the curve, and it moves it
across a family whose two ends are both standard robust estimators. As `N` grows
`(1 - z/N)^N` tends to `exp(-z)`, which is the Welsch estimator: it never quite
stops pulling. At a finite `N` the shape is Tukey's biweight generalised - the
familiar cubic is this shape at `N = 3` - and it reaches 1 exactly, at
`x = sqrt(2N) * sigma`, beyond which a residual contributes nothing at all and
no derivative at all.

That rejection point is the reason to prefer a small `N`, not a concession for
using one. Sigma already says where a residual stops being ordinary, so a
residual six sigma out is not an observation the fit should still be arguing
with. `order` is **4**, which puts the rejection at `5.66 * sigma`.

The cost agrees, because each squaring carries all of a dual's derivatives with
it and so is linear in the order at both compile and run time:

| `order` | rejection | fit of 1e6 passes | compiling the tests |
| ------- | --------- | ----------------- | ------------------- |
| 4       | 5.66 s    | 0.06 s            | 8.9 s               |
| 8       | 22.6 s    | 0.10 s            | 10.9 s              |
| 16      | 362 s     | 0.20 s            | 14.8 s              |
| 20      | 1448 s    | 0.28 s            | 17.0 s              |

Raising it is one number, but it is a change of curve rather than a refinement
of one - the converged error moves with it, from 0.71664 at 4 to 0.71274 in the
limit - and it is coupled to one thing in the tests. Squaring doubles a relative
error, so the kernel's value carries about `2^order` ulp of rounding; a central
difference divides that by its step, so at 4 the finite-difference check's
`h = 1e-6` sits in the floor at 5e-10, and at 20 it would read the noise as
slope and fail. Both numbers move together.

It costs compile time and a raised evaluation budget. It buys a test that cannot
be built and then not run, and cannot pass on one machine and fail on another.

The suite under [`tests/`](../tests) covers the parts that can be wrong without the
fit *looking* wrong. One file per property group, each built against
[`tests/fixtures/simple_graph.hpp`](../tests/fixtures/simple_graph.hpp):

- **Derivatives against central finite differences**, per node and per
  dimension. This exercises the whole chain at once - seeding, the kernel, both
  accumulations, and the slicing back out - and is the test that would catch a
  wrong index offset.
- **A shared node has one block**, holding the sum of what reaches it through
  each edge - checked against a single-edge graph whose gradient it must
  exactly double.
- **An unlinked node is laid out and left alone.**
- **The error never rises** across 200 passes.
- **An exact symmetry is preserved.** Reflecting both nodes through the
  midpoint of the two measurements leaves the objective unchanged, so a fit
  started on that set must stay on it - held to 1e-12 for 200 passes. A
  converged-position check would pass even if the path were crooked; this
  does not.
- **Momentum with `beta = 0` is plain descent**, held to exact equality rather
  than a tolerance. That is the one way momentum can be wrong while still
  converging: a velocity applied when it should be inert.
- **The velocity accumulates as written**, checked against `-lr * g1` and then
  `-lr * (beta * g1 + g2)` with the gradients read back between passes, so it
  is the accumulation being tested and not the fit.
- **Momentum reaches a threshold in fewer passes** than plain descent at the
  same `lr`. If it ever needed more, the velocity would be fighting the
  gradient rather than compounding it.
- **A builder carries no state.** One `momentum{...}` handed to two graphs
  leaves the second starting from rest however hard the first was run - the
  property the builder-and-instance split exists to give.
- Sigma broadcasting, and keyword order independence.

## Where this departs from the sketch it came from

- A node enters the graph as `key<1>(n1)`, not `node<1>(n1)`. `node` is
  already the base a node derives from, and a class template and a function
  template cannot share a name in one scope - GCC calls it a conflicting
  declaration, clang a redefinition as a different kind of symbol. The bases
  keep the names they had; the entry took the new one.
- `key<1>(n1)` and `link<1, 2>(e1)` take parentheses, not braces: class
  template argument deduction is all or nothing, so an entry that gives its key
  and deduces its target has to be a function.
- Both bases carry their dimension. A CRTP base cannot read
  `Derived::dimension`, because it needs the size to complete its own storage
  and `Derived` is still incomplete at that point.
- `error` returns `auto`, not a named `vector`. Its element types depend on
  which node's dual indices flowed in, so there is no one type to name.
- An edge given one sigma against a two-dimensional error works as written: a
  single value is broadcast across every dimension, by the keyword rather than
  by the type.

The build is warning-free under `-Wall -Wextra` on both GCC and Clang. Two of
those warnings were pre-existing in the vendored duals - an unused index-tag
parameter in `functional/zip.hpp` and `functional/view.hpp`, and an implicit
copy constructor alongside a user-declared assignment in `array.hpp` - and are
fixed here rather than carried, now that the duals live in this repository's
own include tree instead of as a one-file diff against a snapshot. See
[dual-snapshot.md](dual-snapshot.md) for what else has drifted from upstream.

## What it is not

Gradient descent with a bounded step, not Gauss-Newton or Levenberg-Marquardt.
There is no linear solve, no information matrix, and no second-order
information at all - the whole point was to see how far the duals alone carry.
Convergence is correspondingly slow: a small graph takes thousands of passes to
settle where a Gauss-Newton step would arrive in tens. `momentum` buys back
most of an order of magnitude, and does not change the kind of thing this is.

The saturating kernel also means the fit does **not** settle on the
least-squares answer, and should not be read as trying to. Two measurements
placed symmetrically about a point pull the estimates onto that line of
symmetry but not onto the point: a residual of three counts for barely more
than a residual of one, so the far measurement pulls proportionally less. That
is the robustness, seen from the inside, and it is why the symmetry check above
holds the fit to the line rather than to a position.
