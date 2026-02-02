add_runtime_module(mempool_memmgr.c)
target_compile_definitions(${RUNTIME_MODULE_TARGET} PRIVATE LOTTO_MEMMGR_USER)

add_driver_module(mempool_flag.c)
target_compile_definitions(${DRIVER_MODULE_TARGET} PRIVATE LOTTO_MEMMGR_USER)
