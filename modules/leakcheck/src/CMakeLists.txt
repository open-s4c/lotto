add_runtime_module(leakcheck.c)
target_compile_definitions(${RUNTIME_MODULE_TARGET} PRIVATE LOTTO_MEMMGR_USER)
