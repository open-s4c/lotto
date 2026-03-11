set(RUNTIME_MODULE_TYPE_memmgr_user_uafcheck SHARED)
set(RUNTIME_MODULE_TYPE_memmgr_runtime_uafcheck SHARED)

new_module(memmgr_runtime_uafcheck)
add_runtime_module(uafcheck.c uaf_memmgr.c)
target_compile_definitions(${RUNTIME_MODULE_TARGET} PRIVATE LOTTO_MEMMGR_RUNTIME)

new_module(memmgr_user_uafcheck)
add_runtime_module(uafcheck.c uaf_memmgr.c)
target_compile_definitions(${RUNTIME_MODULE_TARGET} PRIVATE LOTTO_MEMMGR_USER)
