# The duals - flat derivative storage

Why [`include/uzu/foundation/dual/`](../include/uzu/foundation/dual) is not
quite the `library-dual` snapshot it started as, and what each departure
measured.

The vendored `library-dual` snapshot with two changes. It is built as
**C++20**, which the snapshot it came from was not.

**Flat derivative storage.** `number` stores its derivatives in a
`std::array<T, sizeof...(Dn)>` indexed by position, instead of a
`std::tuple<dvalue_type<Dn>...>` indexed by type. That is
[`dual/number.hpp`](../include/uzu/foundation/dual/number.hpp) alone, and it is
what the rest of this file is about.

**Usable in a constant expression.** The operations, the functional layer and
`array` are marked `constexpr`, and the index algebra already was. Together
these let a whole fit be evaluated by the compiler, which is what the
optimizer uses to run its tests as `static_assert`s.

The library reaches its functors through `std::invoke`, at all 28 call sites.
That is only possible here because the headers are C++20: `std::invoke` did not
become `constexpr` until then, and under C++17 it was the one thing on the path
that a `static_assert` could not evaluate through.

It is not free. Every operation the compiler folds now walks an extra call
layer, which on the optimizer's compile-time test suite costs about a quarter
of the build - 10.9 s against 8.8 s - and raises the constant-evaluation budget
it needs from roughly 45M operations to 55M, both still well inside the 100M
its build line asks for. Run time and every number produced are unchanged, at
`-O2` the layer disappears entirely, and the headers still compile standalone.

`operations/exp.hpp` is the exception, and stays one. It bottoms out in
`std::exp`, and none of `<cmath>` can run at compile time before C++26, so an
expression that reaches for `exp` is a run-time expression. Nothing in the
library needs it to be otherwise: the optimizer's kernel is built from
multiplications rather than from `exp` precisely so that a whole fit can be
evaluated by the compiler.

`tests/dual_test.cpp` and `tests/dual_array_test.cpp` produce byte-identical
output to before the change.

The scan is a hand-rolled loop rather than `std::find`, and stays one now that
the header is C++20 and `std::find` is `constexpr`. The remaining reason is the
measured one: re-checked under C++20, `std::find` costs 1.96 s of frontend time
on the benchmark below against 1.86 s for the loop, the same 5% it cost before.

## The change

```cpp
// before
template <std::size_t D> struct dvalue_type { T value{1.0}; };
std::tuple<dvalue_type<Dn>...> dvalues_;

return std::get<dvalue_type<D>>(dvalues_).value;

// after
std::array<T, sizeof...(Dn)> dvalues_{seeds()};

static constexpr std::size_t index_of(std::size_t d) {
  const std::size_t keys[]{Dn..., 0};
  for (std::size_t i = 0; i < sizeof...(Dn); ++i) {
    if (keys[i] == d) return i;
  }
  return npos;
}

static_assert(index_of(D) != npos, "index is not tracked by this number");
return dvalues_[index_of(D)];
```

`index_of` is a linear scan the compiler folds to a constant. It replaces a
type-keyed `std::get`, which drags in `__get_helper` and, through the tuple
itself, `_Tuple_impl`, `_Head_base` and the whole constructor-constraint
apparatus (`__and_`, `__or_`, `_TupleConstraints`, `is_constructible`,
`is_convertible`, `is_assignable`).

Order within `Dn...` still does not matter semantically, exactly as before:
`dvalue<D>()` looks the index up rather than taking a position.

## Verified

- `tests/dual_test.cpp`, `tests/dual_array_test.cpp` and
  `benchmarks/bezier_curve_bench.cpp` produce byte-identical output to the
  tuple version.
- `sizeof(number<double, 0, 1, 2>)` is 32 bytes in both.
- `number` becomes **trivially copyable**, which the tuple version was not.

## Result

`benchmarks/bezier_curve_bench.cpp`, GCC 13.3, best of three, interleaved, both
sides built as C++17. `-fsyntax-only` isolates the frontend:

| POINTS | tuple frontend | array frontend | | tuple -O2 | array -O2 | |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 4  |  1.77 s | 0.86 s | 2.1x |  3.48 s | 1.65 s | 2.1x |
| 8  |  3.31 s | 1.26 s | 2.6x |  6.26 s | 2.96 s | 2.1x |
| 12 |  5.13 s | 1.80 s | 2.9x |  8.49 s | 4.33 s | 2.0x |
| 16 |  8.47 s | 2.50 s | 3.4x | 12.67 s | 5.62 s | 2.3x |
| 20 |  9.81 s | 3.32 s | 3.0x | 15.95 s | 7.33 s | 2.2x |
| 24 | 12.49 s | 3.72 s | 3.4x | 20.79 s | 8.33 s | 2.5x |

**Half the compile time, and it scales better.** The frontend grows x7.0 from 4
to 24 points with the tuple and x4.3 with the array, so the gap widens with the
problem size: 2.1x at four points, 2.5x at twenty-four.

Template instantiations at 16 points, from clang's `-ftime-trace
-ftime-trace-granularity=0`:

| | tuple | array |
| --- | ---: | ---: |
| **total** | **122 307** | **37 668** |
| `std::__and_` | 8425 | 1403 |
| `std::_Tuple_impl` | 7993 | 614 |
| `std::_Head_base` | 7418 | 478 |
| `std::_TupleConstraints` | 4274 | 246 |
| `std::__or_` | 4161 | 691 |
| `std::get` | 3836 | 366 |
| `std::__get_helper` | 3702 | 232 |
| `std::is_convertible` | 3604 | 162 |
| `std::is_constructible` | 3580 | 138 |
| `std::is_assignable` | 3559 | 89 |
| `dual::binary_operation` | 4767 | 4767 |
| `dual::impl::indices_filter` | 889 | 889 |

70% of all template instantiations disappear. The library's own templates are
untouched - `binary_operation` and `indices_filter` are identical, because
nothing about the operations or the index algebra changed.

### The emitted code is the same

At 16 points the object file's `.text` is 46 686 bytes with the tuple and
46 601 with the array - a 0.2% difference. Yet the codegen half of the compile
also roughly halved. The backend was spending its time inlining away thousands
of tiny tuple accessors to arrive at the same machine code.

That is the summary of the whole experiment: the tuple bought nothing at
runtime and cost half the build.

### Why the lookup returns a sentinel

`index_of` returns `npos` for a missing index rather than a
`std::optional<std::size_t>`. The optional reads better and was tried; it is
not free. `-O2` at 24 points:

| lookup | | |
| --- | ---: | ---: |
| `std::size_t` + `npos`, bound to a `constexpr` local | 10.31 s | - |
| `optional`, bound to a `constexpr` local | 11.09 s | +7.6% |
| `optional`, `consteval` | 11.64 s | +12.9% |
| `optional`, used as `dvalues_[*index_of(D)]` | 12.32 s | +19.5% |

Almost none of that is instantiation work - in the optional build
`std::optional` accounted for 5 instantiations out of 37 719, against 37 668
here. It is the optimiser folding away a wrapper that
gets materialised and unwrapped at every use.

Two things carry over from that experiment even though the optional does not.
Binding the lookup to a `constexpr` local is worth about 11 points on its own,
so the call sites do that:

```cpp
constexpr std::size_t at = index_of(D);
return dvalues_[at];
```

And `consteval` is not needed: with the result bound to a `constexpr` local
the scan is folded either way, and plain `constexpr` keeps the helper usable
in an ordinary constant expression.

## What is left, profiled

Clang `-ftime-trace -ftime-trace-granularity=0` on these headers at 16 points.
Frontend and backend split 56/44. Template instantiation accounts for 5846 ms
of self time, and it is spread like this:

| family | count | self ms | ms each |
| --- | ---: | ---: | ---: |
| `dual::number` | 5113 | 1059 | 0.207 |
| `dual::impl::indices_filter` | 889 | **868** | **0.976** |
| `dual::binary_operation` | 4767 | 793 | 0.166 |
| `dual::detail::curve_line_value` | 33 | 468 | 14.186 |
| `dual::make_duo` | 1707 | 130 | 0.076 |
| all `std::tuple` leftovers | | 675 | |

Grouped by self time over the whole compile: frontend phases outside
instantiation 33%, backend 27%, this library's own templates 24%, other
`std::` 11%, tuple leftovers 5%.

**`indices_filter` is the most expensive template in the library per
instantiation** - 0.98 ms, five times `dual::number` and six times
`binary_operation`. There are only 889 of them, and together with
`intersection`, `difference` and `indices_concat` the index-set algebra is
1001 ms, **17% of all template instantiation time**.

That corrects the reasoning that used to sit here, and which
[dual-snapshot.md](dual-snapshot.md) still records: the index algebra was
argued not to pay because it was 889 instantiations out of 122 307. Counts are
not cost.

## And it still does not pay to replace

The conclusion survives the corrected reasoning. Dropping the one-pass
`consteval` merge from [`dual_merge`](https://github.com/lcmonteiro/explorer/pull/4)
on top of this storage, so it removes the 1001 ms rather than being diluted by
the tuple, and measuring the frontend where the change lives, best and median
of five:

| POINTS | array | + merge | |
| ---: | ---: | ---: | ---: |
| 16 | 4.38 s | 4.71 s | 0.93x |
| 24 | 6.34 s | 6.94 s | 0.91x |

7 to 9% **slower**. The trace says why: `merge` costs about 1.04 ms per
instantiation against `indices_filter`'s 0.98 ms - constant-evaluating a
function that sorts two arrays is no cheaper than instantiating the fold it
replaces - and it adds 1676 `slice` instantiations to rebuild `indices<...>`
from the result.

So the index algebra is expensive, and the obvious replacement is not cheaper.

## The backend, opened up

44% of the build, and it turns out to be one thing:

| pass | events | self ms | |
| --- | ---: | ---: | ---: |
| `InlinerPass` | 15 317 | 864 | 17% |
| `CodeGen Function` | 15 777 | 813 | 16% |
| `SROAPass` | 46 003 | 554 | 11% |
| `InstCombinePass` | 76 741 | 390 | 8% |
| `GVNPass` | 15 317 | 204 | 4% |

**15 777 functions are emitted, and 15 317 of them go through the whole
per-function pass pipeline only to be inlined away.** 13 359 of them belong to
a `number` type:

| emitted for a number type | count |
| --- | ---: |
| member functions other than the ones below | 8246 |
| `dvalue<D>()` get and set | 3470 |
| constructors | 822 |
| `value()` | 821 |

There are **411 distinct `number` specialisations** in this translation unit,
so each one costs about **32 emitted functions**. Their widths say where they
come from:

| indices tracked | 1 | 2 | 3-7 | 13 | 14...28 |
| --- | ---: | ---: | ---: | ---: | ---: |
| distinct types | 28 | 192 | 32 each | 16 | 1 each |

That tail of one type per width from 14 to 28 is the `summation` fold in
`fit`: each term merges one more point index into the accumulator, so every
step of the fold has its own type, and each of those types emits its own
~32 functions.

## So there is one bottleneck, not three

The frontend cost (`dual::number` 1059 ms, `binary_operation` 793 ms), the
index algebra that produces the types (`indices_filter` 868 ms), and the
entire backend are all the same phenomenon: **every distinct index set is a
distinct type**, and each type costs an instantiation, a set of operators, and
about 32 emitted functions that exist only to be inlined away.

Storage was worth 2x because it removed a constant factor per type - a tuple
each. What is left is the *number of types*, and no change to the storage or
to the index algorithm touches it: giving the result of a binary operation a
canonically sorted index set, so that the same set cannot appear under two
orderings, leaves the count at exactly 411 distinct `number` types, because
this expression tree never reaches one set by two orderings in the first
place.

Collapsing the count needs the dense form, where every element of a
collection carries the same index set and the fold's chain of 16 widening
types becomes one. That buys the compile time back with runtime, which is the
trade this storage exists to avoid.

## Re-running the measurements

The benchmarks are CMake targets, off by default because what they measure is
how long the compiler takes, so building one is the whole measurement:

```sh
cmake -S . -B build -DUZU_BUILD_BENCHMARKS=ON -DUZU_BENCHMARK_POINTS=16
cmake --build build --target uzu_bezier_curve_bench
```

`UZU_BENCHMARK_POINTS` is the `POINTS` the tables above sweep, and
`UZU_BENCHMARK_COEFFS` the `COEFFS` of the dense regression probe. To isolate
the frontend as the tables do, build the translation unit directly:

```sh
g++ -std=c++20 -O2 -Iinclude -DPOINTS=16 -fsyntax-only \
    benchmarks/bezier_curve_bench.cpp
```
