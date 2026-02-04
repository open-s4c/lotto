add_runtime_module(state.c handler.c module.c)
add_driver_module(state.c flags.c module.c)
target_link_libraries(${DRIVER_MODULE_TARGET} PRIVATE m)
