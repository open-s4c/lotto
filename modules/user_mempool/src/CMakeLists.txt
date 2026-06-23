set(RUNTIME_MODULE_TYPE_user_mempool-memmgr_user PLUGIN)
set(RUNTIME_MODULE_TYPE_user_mempool-memmgr_runtime PLUGIN)
set(DRIVER_MODULE_TYPE_user_mempool-memmgr_user PLUGIN)

# Remove stale outputs from the pre-rename memmgr_* naming scheme so module
# scans do not load the same plugin twice from an incremental build tree.
file(REMOVE
     "${LOTTO_MODULE_BUILD_DIR}/lotto-driver-memmgr_user_mempool${CMAKE_SHARED_LIBRARY_SUFFIX}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-memmgr_user_mempool${CMAKE_SHARED_LIBRARY_SUFFIX}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-dbg-memmgr_user_mempool${CMAKE_SHARED_LIBRARY_SUFFIX}")

new_module(user_mempool-memmgr_user)
add_runtime_module(mempool_memmgr.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_USER)

add_driver_module(mempool_flag.c)
driver_module_compile_definitions(PRIVATE LOTTO_MEMMGR_USER)
