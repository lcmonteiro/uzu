# dual - vendored snapshot of library-dual

The C++17 dual number library from
[`lcmonteiro/library-dual`](https://github.com/lcmonteiro/library-dual),
copied here so the experiments in this repo can use it.

**Snapshot:** `bf06f08`, byte-identical to upstream - no local patches.

Everything under `functional/`, `helpers/`, `operations/`, plus `array.hpp`,
`number.hpp`, `operations.hpp` and `functional.hpp`, is upstream - as is the
`dual.hpp` umbrella, which sits one level up in the parent folder.
`test_solver.hpp` and `types/` are local. The experiment translation units -
`test.cpp`, `test_array.cpp` and the benches - sit one level up, in the parent
folder, so that this one holds the library alone.

## Building

Includes are rooted at `dual/`, so the include path is the *parent*
directory - which is where the translation units live:

```sh
g++ -std=c++17 -O2 -I. test.cpp -o test              # from the parent
g++ -std=c++17 -O2 -I. test_array.cpp -o test_array  # from the parent
```

`../compile_flags.txt` carries the same flags for clangd, and covers this
folder too.

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

## Re-syncing

Since there are no local patches, refreshing is a straight copy:

```sh
cp -r /path/to/library-dual/include/dual/. .
cp /path/to/library-dual/include/dual.hpp ..
```

The layout mirrors upstream's: the umbrella one level above the folder, so
`include/dual.hpp` and `include/dual/` each land where they came from and
the umbrella is included as `"dual.hpp"`.

## Compile time

`bench_bezier_curve.cpp` measures `bezier_curve::fit` against the number of
points being fitted. `fit` seeds one dual index per control point (6 for x, 6
for y) plus one per fitted point, so the derivative set is `12 + POINTS` wide
and the error expression merges all of it.

```sh
g++ -std=c++17 -O2 -I. -DPOINTS=12 -c bench_bezier_curve.cpp   # from the parent
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
folder does. And the codegen half needs a different
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
  containers, so `test_array.cpp` folds with `dual::apply` instead.
- The headers moved: `algorithms.hpp` -> `functional.hpp`,
  `print.hpp` -> `helpers/print.hpp`, `vector.hpp` -> `array.hpp`.
- Includes are rooted at `dual/` rather than relative.

## Status

`test.cpp` and `test_array.cpp` build and run, as do the three benches.

A fourth experiment, `test_bezier_curve.cpp`, is not carried here: it draws its
output through a `d2/plot.hpp` that belongs to the repository this snapshot came
from, and there is no plotting to depend on in this one.
