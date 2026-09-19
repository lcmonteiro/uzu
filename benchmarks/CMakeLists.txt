# The compile-time probes. These are not tests and are not run by ctest: what they measure is how
# long the compiler takes, so building one is the whole measurement.
#
#   cmake -S . -B build -DUZU_BUILD_BENCHMARKS=ON -DUZU_BENCHMARK_POINTS=16
#   cmake --build build --target uzu_bezier_curve_bench
#
# See docs/dual-storage.md for the numbers these produced and what they were read for.

set(UZU_BENCHMARK_POINTS 12 CACHE STRING "Control points for the bezier benchmarks")
set(UZU_BENCHMARK_COEFFS 16 CACHE STRING "Coefficients for the regression benchmark")

foreach(source bezier_curve_bench.cpp bezier_runtime_bench.cpp regression_bench.cpp)
  get_filename_component(name ${source} NAME_WE)
  set(target uzu_${name})

  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE uzu::uzu)
  uzu_warnings(${target})
  target_compile_definitions(${target} PRIVATE
    POINTS=${UZU_BENCHMARK_POINTS}
    COEFFS=${UZU_BENCHMARK_COEFFS})
endforeach()
