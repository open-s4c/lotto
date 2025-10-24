add_library(lotto_drop_engine SHARED handler.c state.c interceptor.c category.c)

set_target_properties(lotto_drop_engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                   "${LOTTO_PLUGIN_BUILD_DIR}")

target_include_directories(
    lotto_drop_engine PRIVATE ../../../src/include ../../../include ../include)

target_link_libraries(lotto_drop_engine PRIVATE engine)

add_library(lotto_drop_cli SHARED flag.c state.c category.c)

set_target_properties(lotto_drop_cli PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                "${LOTTO_PLUGIN_BUILD_DIR}")

target_include_directories(lotto_drop_cli PRIVATE ../../../src/include
                                                  ../../../include ../include)

install(
    TARGETS lotto_drop_engine lotto_drop_cli
    DESTINATION DESTINATION
    "${LOTTO_PLUGIN_INSTALL_DIR}")
