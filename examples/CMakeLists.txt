# The examples are built by default, and by CI, so that what the README claims about them stays
# true. They are not registered with ctest: an example's job is to be read and run by hand, and
# `compile_time_fit` does its checking with `static_assert` anyway -- building it *is* running it.
#
# It needs the raised constant-evaluation budget on Clang, whose default is about a million steps,
# and none at all on GCC, whose default is thirty times that. uzu_constexpr_budget() asks for
# whichever the compiler in use spells, so the example builds the same way everywhere.

foreach(source compile_time_fit.cpp)
  get_filename_component(name ${source} NAME_WE)
  set(target uzu_${name})

  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE uzu::uzu)
  uzu_warnings(${target})
  uzu_constexpr_budget(${target})
endforeach()
