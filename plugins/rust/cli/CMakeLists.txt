add_library(lotto_rust_cli SHARED init_cli.c)
set_target_properties(lotto_rust_cli PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                "${LOTTO_PLUGIN_BUILD_DIR}")
target_include_directories(
    lotto_rust_cli PRIVATE ${PROJECT_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR}/src/include)
target_link_libraries(lotto_rust_cli PRIVATE rusty_cli)

install(TARGETS lotto_rust_engine lotto_rust_cli
        DESTINATION "${LOTTO_PLUGIN_INSTALL_DIR}")
