add_library(lotto_qemu_engine SHARED state.c)

set_target_properties(lotto_qemu_engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                   "${LOTTO_PLUGIN_BUILD_DIR}")

target_include_directories(
    lotto_qemu_engine PRIVATE ../../../src/include ../../../include ../include)

target_link_libraries(lotto_qemu_engine PRIVATE engine)

add_library(lotto_qemu_cli SHARED flag.c)

set_target_properties(lotto_qemu_cli PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                "${LOTTO_PLUGIN_BUILD_DIR}")

target_include_directories(lotto_qemu_cli PRIVATE ../../../src/include
                                                  ../../../include ../include)

install(
    TARGETS lotto_qemu_engine lotto_qemu_cli
    DESTINATION DESTINATION
    "${LOTTO_PLUGIN_INSTALL_DIR}")
