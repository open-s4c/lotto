add_runtime_module(module.c profile_emit.c subscribers.c)
runtime_module_include_directories(
    PRIVATE ${PROJECT_SOURCE_DIR}/deps/qemu/include/qemu
    ${CMAKE_BINARY_DIR}/include /usr/include/glib-2.0
    /usr/lib/x86_64-linux-gnu/glib-2.0/include)
