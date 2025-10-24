add_library(lotto_rust_engine SHARED init_engine.c)
set_target_properties(lotto_rust_engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                   "${LOTTO_PLUGIN_BUILD_DIR}")
target_include_directories(
    lotto_rust_engine PRIVATE ${PROJECT_SOURCE_DIR}/include
                              ${PROJECT_SOURCE_DIR}/src/include)
target_link_libraries(lotto_rust_engine PRIVATE engine rusty_engine)
