add_library(plugin-snapshot SHARED plugin_snapshot.c)
target_include_directories(
    plugin-snapshot
    PRIVATE ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu
            ${PROJECT_SOURCE_DIR}/modules/qemu/include
            ${PROJECT_SOURCE_DIR}/include)
target_include_directories(plugin-snapshot PUBLIC ${CMAKE_BINARY_DIR}/include)
target_include_directories(
    plugin-snapshot PUBLIC /usr/include/glib-2.0
                           /usr/lib/x86_64-linux-gnu/glib-2.0/include)
target_link_libraries(plugin-snapshot PRIVATE capstone)
add_dependencies(plugin-snapshot capstone)

set_target_properties(plugin-snapshot PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                 "${PROJECT_BINARY_DIR}/bin")

install(
    TARGETS plugin-snapshot
    DESTINATION "${LOTTO_MODULE_INSTALL_DIR}"
    COMPONENT runtime)
