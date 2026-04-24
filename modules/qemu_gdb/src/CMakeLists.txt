file(GLOB QEMU_GDB_SRCS CONFIGURE_DEPENDS
     ${PROJECT_SOURCE_DIR}/modules/qemu_gdb/src/gdb/*.c
     ${PROJECT_SOURCE_DIR}/modules/qemu_gdb/src/gdb/handling/*.c)

add_runtime_module(state.c module.c subscribers.c ${QEMU_GDB_SRCS})
runtime_module_compile_options(PRIVATE -O0 -g)
runtime_module_compile_definitions(PRIVATE _GNU_SOURCE LOGGER_PREFIX="gdb")
runtime_module_include_directories(
    PRIVATE
    ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu
    ${PROJECT_SOURCE_DIR}/modules/qemu/include
    ${PROJECT_SOURCE_DIR}/modules/qemu_gdb/include
    ${PROJECT_SOURCE_DIR}/modules/qemu_gdb/src
    ${CMAKE_BINARY_DIR}/include
    /usr/include/glib-2.0
    /usr/lib/x86_64-linux-gnu/glib-2.0/include)
runtime_module_link_libraries(PRIVATE elf)
add_dependencies(lotto-runtime-qemu_gdb blob-aarch64-core
                 blob-aarch64-fpu blob-system-registers blob-target)
add_driver_module(state.c flags.c module.c)
