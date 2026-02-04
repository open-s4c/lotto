add_library(plugin-snapshot SHARED plugin_snapshot.c)
target_include_directories(
    plugin-snapshot PRIVATE ${PROJECT_SOURCE_DIR}/src/include
                            ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu)

target_include_directories(plugin-snapshot PUBLIC ${CMAKE_BINARY_DIR}/include)
# required for QEMUv9
target_include_directories(
    plugin-snapshot PUBLIC /usr/include/glib-2.0
                           /usr/lib/x86_64-linux-gnu/glib-2.0/include)

target_link_libraries(plugin-snapshot PRIVATE capstone)

add_dependencies(plugin-snapshot capstone)

set_target_properties(plugin-snapshot PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                 "${QLOTTO_MODULE_BUILD_DIR}")

install(
    TARGETS plugin-snapshot
    DESTINATION DESTINATION
    "${QLOTTO_MODULE_INSTALL_DIR}")
