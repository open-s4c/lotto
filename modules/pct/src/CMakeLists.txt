add_runtime_module(state.c handler.c module.c)
add_driver_module(state.c flags.c module.c)
driver_module_link_libraries(PRIVATE m)
