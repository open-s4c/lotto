set(RUNTIME_MODULE_TYPE_uafcheck-memmgr_user PLUGIN)
set(RUNTIME_MODULE_TYPE_uafcheck-memmgr_runtime PLUGIN)
set(DRIVER_MODULE_TYPE_uafcheck-memmgr_user PLUGIN)

new_module(uafcheck-memmgr_runtime)
add_runtime_module(state.c uafcheck.c uaf_memmgr.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_RUNTIME)

new_module(uafcheck-memmgr_user)
add_runtime_module(state.c uafcheck.c uaf_memmgr.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_USER)

add_driver_module(state.c flags.c)
