# Every test here is a translation unit of static_asserts, so the build *is* the test run: a
# failed property is a compile error naming itself, at the line that states it. Each one gets an
# executable all the same, so that ctest reports the suite by name and a build that was not run
# cannot be mistaken for a suite that passed.
#
# The graph tests need the raised constant-evaluation budget (see uzu_constexpr_budget in the top
# level CMakeLists.txt); the dual ones are ordinary run-time programs and do not.

set(UZU_COMPILE_TIME_TESTS
  graph_derivatives_test.cpp
  graph_layout_test.cpp
  graph_descent_test.cpp
  graph_algorithm_test.cpp
  graph_contracts_test.cpp
  graph_enable_test.cpp
  keyed_layout_test.cpp
  dual_derivatives_test.cpp
  keywords_test.cpp
  trajectory_compile_time_test.cpp)

set(UZU_RUNTIME_TESTS
  dual_test.cpp
  dual_array_test.cpp
  optimization_trajectory_test.cpp)

# A thousand passes over a four-pose chain runs past the budget the others need, so this one asks
# for ten times it. The limit is a diagnostic rather than a reservation, so raising it for one
# translation unit costs nothing anywhere else -- which is why it is raised here and not for all.
set(UZU_LARGE_BUDGET_TESTS trajectory_compile_time_test.cpp)
set(UZU_LARGE_BUDGET 1000000000)

foreach(source IN LISTS UZU_COMPILE_TIME_TESTS UZU_RUNTIME_TESTS)
  get_filename_component(name ${source} NAME_WE)
  set(target uzu_${name})

  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE uzu::uzu)
  # Rooted at the repository so that a test says `#include "tests/fixtures/simple_graph.hpp"`,
  # the same spelling the README uses.
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
  uzu_warnings(${target})

  if(${source} IN_LIST UZU_LARGE_BUDGET_TESTS)
    uzu_constexpr_budget(${target} ${UZU_LARGE_BUDGET})
  elseif(${source} IN_LIST UZU_COMPILE_TIME_TESTS)
    uzu_constexpr_budget(${target})
  endif()

  add_test(NAME ${name} COMMAND ${target})
endforeach()
