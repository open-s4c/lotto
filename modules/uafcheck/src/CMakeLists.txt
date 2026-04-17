set(RUNTIME_MODULE_TYPE_uafcheck-memmgr_user PLUGIN)
set(RUNTIME_MODULE_TYPE_uafcheck-memmgr_runtime PLUGIN)

new_module(uafcheck-memmgr_runtime)
add_runtime_module(uafcheck.c uaf_memmgr.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_RUNTIME)

new_module(uafcheck-memmgr_user)
add_runtime_module(uafcheck.c uaf_memmgr.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_USER)
