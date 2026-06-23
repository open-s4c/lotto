set(RUNTIME_MODULE_TYPE_uafcheck-memmgr_user PLUGIN)
set(RUNTIME_MODULE_TYPE_uafcheck-memmgr_runtime PLUGIN)
set(DRIVER_MODULE_TYPE_uafcheck-memmgr_user PLUGIN)

# Remove stale outputs from the pre-rename memmgr_* naming scheme so module
# scans do not load the same plugin twice from an incremental build tree.
file(REMOVE
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-memmgr_user_uafcheck${CMAKE_SHARED_LIBRARY_SUFFIX}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-dbg-memmgr_user_uafcheck${CMAKE_SHARED_LIBRARY_SUFFIX}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-memmgr_runtime_uafcheck${CMAKE_SHARED_LIBRARY_SUFFIX}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-dbg-memmgr_runtime_uafcheck${CMAKE_SHARED_LIBRARY_SUFFIX}")

new_module(uafcheck-memmgr_runtime)
add_runtime_module(state.c uafcheck.c uaf_memmgr.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_RUNTIME)

new_module(uafcheck-memmgr_user)
add_runtime_module(state.c uafcheck.c uaf_memmgr.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_USER)

add_driver_module(state.c flags.c)
