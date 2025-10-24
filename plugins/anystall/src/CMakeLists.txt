add_library(lotto_anystall_engine SHARED handler.c state.c)

set_target_properties(
    lotto_anystall_engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                     "${LOTTO_PLUGIN_BUILD_DIR}")

target_include_directories(
    lotto_anystall_engine PRIVATE ../../../src/include ../../../include
                                  ../include)

target_link_libraries(lotto_anystall_engine PRIVATE engine)

add_library(lotto_anystall_cli SHARED flag.c state.c)

set_target_properties(lotto_anystall_cli PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                    "${LOTTO_PLUGIN_BUILD_DIR}")

target_include_directories(
    lotto_anystall_cli PRIVATE ../../../src/include ../../../include ../include)

install(
    TARGETS lotto_anystall_engine lotto_anystall_cli
    DESTINATION DESTINATION
    "${LOTTO_PLUGIN_INSTALL_DIR}")
