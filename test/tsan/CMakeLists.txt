add_compile_definitions(_GNU_SOURCE)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
add_executable(tsan_interface_test test_tsan_atomics.c)
target_link_libraries(tsan_interface_test sys.o memmgr_runtime.o)
