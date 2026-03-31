add_runtime_module(
    state.c
    module.c
    perf.c
    qlotto.c
    frontend/event.c
    frontend/intercept_translation.c
    frontend/interceptor.c
    frontend/perf.c
    frontend/qemu_connector.c
    frontend/stubs.c
    frontend/udf_trace.c)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0)
runtime_module_compile_options(PRIVATE -O0 -g)
runtime_module_compile_definitions(PRIVATE _GNU_SOURCE)
runtime_module_include_directories(
    PRIVATE ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu
    ${PROJECT_SOURCE_DIR}/modules/qemu/qlotto/include PUBLIC
    ${CMAKE_BINARY_DIR}/include)
runtime_module_link_libraries(PRIVATE capstone PkgConfig::GLIB2)
add_driver_module(state.c flags.c cmd.c module.c)
