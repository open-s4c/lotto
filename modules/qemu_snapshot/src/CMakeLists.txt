add_runtime_module(module.c subscribers.c
                   ${PROJECT_SOURCE_DIR}/modules/qemu_snapshot/qlotto/snapshot/plugin_snapshot.c)
runtime_module_include_directories(
    PRIVATE ${PROJECT_SOURCE_DIR}/deps/qemu/include
            ${PROJECT_SOURCE_DIR}/modules/qemu/include
            ${PROJECT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include
            /usr/include/glib-2.0
            /usr/lib/x86_64-linux-gnu/glib-2.0/include)
runtime_module_link_libraries(PRIVATE capstone)
add_dependencies(lotto-runtime-qemu_snapshot capstone)
