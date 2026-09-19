cmake_minimum_required(VERSION 3.24)
project(uzu LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(UZU_BUILD_TESTS "Build uzu tests" ON)
option(UZU_BUILD_EXAMPLES "Build uzu examples" ON)
option(UZU_BUILD_BENCHMARKS "Build uzu benchmarks" OFF)

# ---------------------------------------------------------------------------
# uzu library (header-only, no dependencies)
# ---------------------------------------------------------------------------
add_library(uzu INTERFACE)
target_compile_features(uzu INTERFACE cxx_std_20)
target_include_directories(uzu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(uzu::uzu ALIAS uzu)

# ---------------------------------------------------------------------------
# The constant-evaluation budget
#
# uzu is built to be usable inside a constant expression, and the test suite takes that literally:
# every assertion is a static_assert and every fit is evaluated by the compiler. That runs past the
# default budget for constant evaluation, which is a diagnostic rather than a hard limit -- so the
# flag is what makes the suite buildable at all, not a tuning knob.
#
# It is not put on uzu::uzu itself: a consumer that only evaluates its fits at run time does not
# need it, and MSVC's spelling of it is different again. Whoever wants a compile-time fit asks for
# the budget the same way the tests below do. See uzu_constexpr_budget() for the per-compiler flag.
# ---------------------------------------------------------------------------
function(uzu_constexpr_budget target)
  set(budget 100000000)
  if(ARGC GREATER 1)
    set(budget ${ARGV1})
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} PRIVATE -fconstexpr-steps=${budget})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PRIVATE -fconstexpr-ops-limit=${budget})
  elseif(MSVC)
    target_compile_options(${target} PRIVATE /constexpr:steps${budget})
  endif()
endfunction()

# ---------------------------------------------------------------------------
# Warnings, shared by the tests and the benchmarks
# ---------------------------------------------------------------------------
function(uzu_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# Tests and benchmarks
# ---------------------------------------------------------------------------
if(UZU_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(UZU_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(UZU_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
