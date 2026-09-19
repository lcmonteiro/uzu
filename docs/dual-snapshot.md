# The duals - where they came from

The C++17 dual number library from
[`lcmonteiro/library-dual`](https://github.com/lcmonteiro/library-dual),
vendored here so the optimizer can use it.

**Snapshot:** `bf06f08`. It was byte-identical to upstream when it was taken;
it no longer is. Read this file for the provenance and
[dual-storage.md](dual-storage.md) for what has changed since and what each
change measured.

## Where it lives now

Upstream's layout was the library under `dual/` with the umbrella one level
above it. This repository puts the whole thing under one include root, the way
every other module here sits:

| upstream | here |
| --- | --- |
| `dual.hpp` | [`include/uzu/foundation/dual.hpp`](../include/uzu/foundation/dual.hpp) |
| `dual/number.hpp`, `dual/array.hpp` | `include/uzu/foundation/dual/` |
| `dual/operations/`, `dual/functional/` | `include/uzu/foundation/dual/{operations,functional}/` |
| `dual/helpers/indices.hpp` | `include/uzu/foundation/dual/indices.hpp` |
| `dual/helpers/print.hpp` | `include/uzu/foundation/dual/print.hpp` |
| `dual/helpers/operations.hpp` | `include/uzu/foundation/dual/operations/base.hpp` |
| `dual/types/` (local, not upstream) | `include/uzu/foundation/types/` |
| `test_solver.hpp` (local, not upstream) | [`tests/helpers/reference_solver.hpp`](../tests/helpers/reference_solver.hpp) |
| `test.cpp`, `test_array.cpp` | `tests/dual_test.cpp`, `tests/dual_array_test.cpp` |
| the benches | `benchmarks/` |

Includes are rewritten to match, and `#pragma once` is replaced by include
guards - both repository-wide conventions rather than anything about the
duals. The `dual::` namespace is untouched: a program using both halves still
says which one it is reaching for.

## Building

There is no separate build any more. The headers are part of `uzu::uzu`, and
the translation units that were the experiments are CMake targets:

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure -R dual
```

## Upstream fixes that came with this snapshot

The previous copy here needed four one-line fixes to make the headers usable
on their own. They were sent upstream and merged as
[library-dual#8](https://github.com/lcmonteiro/library-dual/pull/8), so this
snapshot carries them from upstream rather than as local patches:

| file | fix |
| --- | --- |
| `helpers/indices.hpp` | `#include <cstddef>` - it uses unqualified `size_t`, so `#include "dual.hpp"` as the first include of a translation unit did not compile |
| `functional/apply.hpp` | `#include <functional>` - it calls `std::invoke` |
| `functional/zip.hpp` | `#include <algorithm>` - it calls `std::min` |
| `functional/zip.hpp` | `"helpers/indices.hpp"` -> `"dual/helpers/indices.hpp"` - the relative path resolved to `dual/functional/helpers/`, which does not exist |

The library's own test suite does not catch the first three because every test
includes `<gtest/gtest.h>` and `<cmath>` first, which drag the missing
declarations in.

Every header compiles standalone under GCC 13.3 and Clang 18.1.

## Local fixes made while moving

Moving the headers under one include root put every one of them in the build
for the first time, under `-Wall -Wextra` and as a standalone translation unit.
That surfaced four things, all fixed here:

| file | fix |
| --- | --- |
| `functional/view.hpp`, `functional/zip.hpp` | the index sequence is a deduction tag and is never read, so it is left unnamed rather than named and ignored (`-Wunused-parameter`) |
| `array.hpp` | the copy constructor is declared alongside the user-declared copy assignment, which otherwise deprecates it (`-Wdeprecated-copy`). Behaviour is unchanged: move stays suppressed, as it already was |
| `types/bezier_curve_5.hpp` | `<algorithm>`, `<cmath>`, `<cstddef>`, `<limits>`, `<numeric>` and `<utility>` added — it used `std::abs`, `std::clamp`, `size_t`, `std::numeric_limits` and `std::index_sequence` without them |
| `types/bezier_curve_5.hpp` | `iota<T, N>()` added. The header called it without ever declaring it; GCC rejects the non-dependent name outright, and Clang only accepted it because the member initializer was never instantiated. It is spelled the way `bezier_curve.hpp`, which this was extracted from, spells it |

`types/` is local rather than upstream, so the last two are this repository's
own bugs rather than drift from `library-dual`. Every header now compiles
standalone under GCC 13.3 and Clang 18.1, warning-free.

## Re-syncing

Refreshing is no longer a straight copy. Upstream's files land at the paths in
the table above rather than at their own, their includes have to be rewritten
to the `uzu/foundation/dual/` root, and the local changes recorded in
[dual-storage.md](dual-storage.md) - the flat derivative storage, the
`constexpr` marking, and the C++20 move - have to be carried back on top. A
re-sync is a merge, and the two documents together are the list of what has to
survive it.

## Compile time

`benchmarks/bezier_curve_bench.cpp` measures `bezier_curve::fit` against the number of
points being fitted. `fit` seeds one dual index per control point (6 for x, 6
for y) plus one per fitted point, so the derivative set is `12 + POINTS` wide
and the error expression merges all of it.

```sh
cmake -S . -B build -DUZU_BUILD_BENCHMARKS=ON -DUZU_BENCHMARK_POINTS=12
cmake --build build --target uzu_bezier_curve_bench
```

GCC 13.3, best of two, `-fsyntax-only` isolating the frontend:

| POINTS | dim | frontend | -O2 total | codegen |
| ---: | ---: | ---: | ---: | ---: |
| 4  | 16 |  2.82 s |  4.62 s |  1.80 s |
| 8  | 20 |  5.10 s |  8.45 s |  3.36 s |
| 12 | 24 |  6.86 s | 11.83 s |  4.97 s |
| 16 | 28 |  9.57 s | 16.04 s |  6.47 s |
| 20 | 32 | 12.15 s | 20.06 s |  7.91 s |
| 24 | 36 | 14.62 s | 25.39 s | 10.77 s |

**Linear, at about one second of compile time per fitted point.** Clang 18.1
agrees: 6.41 s at 4 points, 26.36 s at 16, and every template family grows
x3.7-4.1 when the point count grows x4. Nothing here is combinatorial - it is
a large constant multiplied by N.

Roughly 58% is frontend and 42% codegen, so this is not purely a
metaprogramming problem.

### Where the frontend time goes

Clang's `-ftime-trace -ftime-trace-granularity=0` at 16 points: 122,307
template instantiations.

| family | count |
| --- | ---: |
| `std::__and_` | 8425 |
| `std::_Tuple_impl` | 7993 |
| `std::_Head_base` | 7420 |
| `dual::number` | 6437 |
| `dual::binary_operation` | 4767 |
| `std::__or_` | 4162 |
| `std::get` | 3836 |
| `std::__get_helper` | 3702 |
| `std::tuple` | 3522 |
| `dual::impl::indices_filter` | 889 |

**About half of all instantiations are libstdc++ tuple plumbing**, not this
library. `number` stores its derivatives in a `std::tuple<dvalue_type<Dn>...>`,
so every distinct index set instantiates a fresh tuple, its recursive
`_Tuple_impl` chain, and the constructor-constraint machinery behind it -
`__and_`, `__or_`, `_TupleConstraints`, `is_constructible`, `is_convertible`,
`is_assignable`. `std::get<dvalue_type<I>>` adds a `__get_helper` walk per
derivative read.

The index-set algebra is **not** the bottleneck: `indices_filter` accounts for
889 instantiations out of 122,307. The cost is in what the index set is used
to build, not in computing it.

Two things follow. Replacing the tuple with a flat `std::array<T, N>` indexed
by position removes the largest block of work outright - that is what this
storage now does. And the codegen half needs a different
answer: at 24 points the compiler is optimising a fully unrolled expression
over 36 derivatives per operation.

One oddity worth knowing: `-O0` is *slower* than `-O2` here (37.7 s against
25.4 s at 24 points). At `-O0` every one of the thousands of tiny inline
functions is emitted as a real function; `-O2` inlines and deletes them.
Do not reach for `-O0` to make these builds faster.

## API drift from the previous snapshot

The copy that used to live here predated several upstream changes; the
experiment files were updated to match:

- `make_vector<D, N>` is now `make_array<D, N>`.
- `product(data, predicate)` used to be the multiplicative twin of
  `summation`. Upstream now uses that name for the cartesian product of
  containers, so `tests/dual_array_test.cpp` folds with `dual::apply` instead.
- The headers moved: `algorithms.hpp` -> `functional.hpp`,
  `print.hpp` -> `helpers/print.hpp`, `vector.hpp` -> `array.hpp`.
- Includes are rooted at `dual/` rather than relative.

## Status

`tests/dual_test.cpp` and `tests/dual_array_test.cpp` build and run under
`ctest`, as do the three benchmarks when `UZU_BUILD_BENCHMARKS=ON`.

A fourth experiment, `test_bezier_curve.cpp`, is not carried here: it draws its
output through a `d2/plot.hpp` that belongs to the repository this snapshot came
from, and there is no plotting to depend on in this one.
