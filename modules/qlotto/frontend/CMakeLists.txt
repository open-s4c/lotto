file(GLOB SRCS *.c)
add_library(plugin-lotto SHARED ${SRCS})
target_compile_options(plugin-lotto PRIVATE -O0 -g)
add_compile_definitions(_GNU_SOURCE)

target_include_directories(
    plugin-lotto PRIVATE ${PROJECT_SOURCE_DIR}/src/include
                         ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu)

target_include_directories(plugin-lotto PUBLIC ${CMAKE_BINARY_DIR}/include)
# required for QEMUv9
target_include_directories(
    plugin-lotto PUBLIC /usr/include/glib-2.0
                        /usr/lib/x86_64-linux-gnu/glib-2.0/include)

target_link_libraries(plugin-lotto PRIVATE capstone lperf.o)

set_target_properties(plugin-lotto PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                              "${QLOTTO_PLUGIN_BUILD_DIR}")
install(
    TARGETS plugin-lotto
    DESTINATION DESTINATION
    "${QLOTTO_PLUGIN_INSTALL_DIR}")
