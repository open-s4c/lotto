add_runtime_module(state.c module.c perf.c handlers.c)
target_include_directories(${RUNTIME_MODULE_TARGET}
                           PRIVATE ${PROJECT_SOURCE_DIR}/modules/qemu/qlotto/include)
target_link_libraries(${RUNTIME_MODULE_TARGET} PRIVATE plugin-lotto
                                                      plugin-gdb-server)
add_driver_module(state.c flags.c cmd.c module.c)
