add_runtime_module(module.c state.c handler.c intercept_mutex.c
                   intercept_pthread_mutex.c)
add_driver_module(state.c flags.c module.c)
