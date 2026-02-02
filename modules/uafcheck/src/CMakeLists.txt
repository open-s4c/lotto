add_runtime_module(uafcheck.c uaf_memmgr.c)
target_compile_definitions(${RUNTIME_MODULE_TARGET} PRIVATE LOTTO_MEMMGR_USER)
