add_subdirectory(xml)

file(GLOB SRCS *.c handling/*.c)

add_library(plugin-gdb-server SHARED ${SRCS})
target_compile_options(plugin-gdb-server PRIVATE -O0 -g)
add_compile_definitions(_GNU_SOURCE)

target_include_directories(
    plugin-gdb-server PRIVATE ${PROJECT_SOURCE_DIR}/src/include
                              ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu)

target_include_directories(plugin-gdb-server PUBLIC ${CMAKE_BINARY_DIR}/include)
# required for QEMUv9
target_include_directories(
    plugin-gdb-server PUBLIC /usr/include/glib-2.0
                             /usr/lib/x86_64-linux-gnu/glib-2.0/include)

target_link_libraries(plugin-gdb-server PRIVATE capstone elf plugin-lotto)

add_dependencies(
    plugin-gdb-server
    capstone
    blob-aarch64-core
    blob-aarch64-fpu
    blob-system-registers
    blob-target
    plugin-lotto)

set_target_properties(plugin-gdb-server PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                   "${QLOTTO_PLUGIN_BUILD_DIR}")

install(
    TARGETS plugin-gdb-server
    DESTINATION DESTINATION
    "${QLOTTO_PLUGIN_INSTALL_DIR}")
