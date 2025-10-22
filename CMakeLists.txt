cmake_minimum_required(VERSION 3.16)

project(lotto VERSION 3.0.0 LANGUAGES C)

set(DICE_TESTS OFF CACHE BOOL "" FORCE)
set(DICE_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(DICE_COVERAGE OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/dice EXCLUDE_FROM_ALL)

add_subdirectory(src/runtime)
add_subdirectory(test/integration)
