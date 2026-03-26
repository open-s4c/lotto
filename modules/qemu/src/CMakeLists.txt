add_runtime_module(state.c
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
target_compile_options(${RUNTIME_MODULE_TARGET} PRIVATE -O0 -g)
target_compile_options(${RUNTIME_VERBOSE_MODULE_TARGET} PRIVATE -O0 -g)
target_compile_definitions(${RUNTIME_MODULE_TARGET} PRIVATE _GNU_SOURCE)
target_compile_definitions(${RUNTIME_VERBOSE_MODULE_TARGET} PRIVATE _GNU_SOURCE)
target_include_directories(
    ${RUNTIME_MODULE_TARGET}
    PRIVATE ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu
            ${PROJECT_SOURCE_DIR}/modules/qemu/qlotto/include
    PUBLIC ${CMAKE_BINARY_DIR}/include)
target_include_directories(
    ${RUNTIME_VERBOSE_MODULE_TARGET}
    PRIVATE ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu
            ${PROJECT_SOURCE_DIR}/modules/qemu/qlotto/include
    PUBLIC ${CMAKE_BINARY_DIR}/include)
target_link_libraries(${RUNTIME_MODULE_TARGET} PRIVATE capstone
                                                      PkgConfig::GLIB2)
target_link_libraries(${RUNTIME_VERBOSE_MODULE_TARGET} PRIVATE capstone
                                                              PkgConfig::GLIB2)
add_driver_module(state.c flags.c cmd.c module.c)
