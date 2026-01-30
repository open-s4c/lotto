add_library(plugin-measure SHARED plugin_measure.c)
target_include_directories(
    plugin-measure PRIVATE ${PROJECT_SOURCE_DIR}/src/include
                           ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu)

target_include_directories(plugin-measure PUBLIC ${CMAKE_BINARY_DIR}/include)
# required for QEMUv9
target_include_directories(
    plugin-measure PUBLIC /usr/include/glib-2.0
                          /usr/lib/x86_64-linux-gnu/glib-2.0/include)

target_link_libraries(plugin-measure PRIVATE capstone)

add_dependencies(plugin-measure capstone)

set_target_properties(plugin-measure PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                "${QLOTTO_PLUGIN_BUILD_DIR}")

install(
    TARGETS plugin-measure
    DESTINATION DESTINATION
    "${QLOTTO_PLUGIN_INSTALL_DIR}")
