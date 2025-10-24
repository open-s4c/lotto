add_library(lotto_rust_runtime SHARED intercept_await.c intercept_spin_loop.c)
set_target_properties(lotto_rust_runtime PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                    "${LOTTO_PLUGIN_BUILD_DIR}")
target_include_directories(
    lotto_rust_runtime PRIVATE ${PROJECT_SOURCE_DIR}/include
                               ${PROJECT_SOURCE_DIR}/src/include)
target_link_libraries(lotto_rust_runtime PRIVATE runtime)
