add_runtime_module(
    module.c
    qlotto.c
    translate_aarch64.c
    interceptors.c
    emit_cpu_context.c
    emit_memaccess.c
    emit_udf_exit.c
    emit_udf_trap.c
    emit_wfe.c
    emit_wfi.c
    qemu_connector.c)
runtime_module_compile_options(PRIVATE -O0 -g)
runtime_module_compile_definitions(PRIVATE _GNU_SOURCE)
runtime_module_include_directories(
    PRIVATE ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu
    ${CMAKE_BINARY_DIR}/include)
runtime_module_link_libraries(PRIVATE capstone PkgConfig::GLIB2)
if(TARGET lotto)
    target_link_libraries(lotto PRIVATE capstone PkgConfig::GLIB2)
endif()
if(TARGET lotto-dbg)
    target_link_libraries(lotto-dbg PRIVATE capstone PkgConfig::GLIB2)
endif()
add_driver_module(flags.c cmd.c module.c)
