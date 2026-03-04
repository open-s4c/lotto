add_runtime_module(module.c state.c handler.c subscribe_mutex.c
                   subscribe_pthread_mutex.c)
add_driver_module(state.c flags.c module.c)
