add_runtime_module(module.c state.c subscribers.c snapshot.c)
runtime_module_include_directories(
    PRIVATE ${PROJECT_SOURCE_DIR}/deps/qemu/include
            ${PROJECT_SOURCE_DIR}/modules/qemu/include
            ${PROJECT_SOURCE_DIR}/modules/qemu_snapshot/include
            ${PROJECT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include
            /usr/include/glib-2.0
            /usr/lib/x86_64-linux-gnu/glib-2.0/include)

add_driver_module(module.c state.c cmd.c)
driver_module_include_directories(
    PRIVATE ${PROJECT_SOURCE_DIR}/modules/qemu_snapshot/include
            ${PROJECT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include)
