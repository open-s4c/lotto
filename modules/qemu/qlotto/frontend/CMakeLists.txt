file(GLOB SRCS *.c)
add_library(plugin-lotto SHARED ${SRCS})
target_compile_options(plugin-lotto PRIVATE -O0 -g)
add_compile_definitions(_GNU_SOURCE)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0)

target_include_directories(
    plugin-lotto PRIVATE ${PROJECT_SOURCE_DIR}/src/include
                         ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu)

target_include_directories(plugin-lotto PUBLIC ${CMAKE_BINARY_DIR}/include)

target_link_libraries(plugin-lotto PRIVATE capstone PkgConfig::GLIB2)
target_link_libraries(plugin-lotto PRIVATE lotto)

set_target_properties(plugin-lotto PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                              "${QLOTTO_MODULE_BUILD_DIR}")
install(
    TARGETS plugin-lotto
    DESTINATION DESTINATION
    "${QLOTTO_MODULE_INSTALL_DIR}")
