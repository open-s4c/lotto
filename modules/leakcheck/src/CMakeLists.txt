set(RUNTIME_MODULE_TYPE_leakcheck-memmgr_user PLUGIN)
set(RUNTIME_MODULE_TYPE_leakcheck-memmgr_runtime PLUGIN)

# Remove stale outputs from the pre-rename memmgr_* naming scheme so module
# scans do not load the same plugin twice from an incremental build tree.
file(REMOVE
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-memmgr_user_leakcheck${CMAKE_SHARED_LIBRARY_SUFFIX}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-dbg-memmgr_user_leakcheck${CMAKE_SHARED_LIBRARY_SUFFIX}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-memmgr_runtime_leakcheck${CMAKE_SHARED_LIBRARY_SUFFIX}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-runtime-dbg-memmgr_runtime_leakcheck${CMAKE_SHARED_LIBRARY_SUFFIX}")

new_module(leakcheck-memmgr_user)
add_runtime_module(leakcheck.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_USER)

new_module(leakcheck-memmgr_runtime)
add_runtime_module(leakcheck.c)
runtime_module_compile_definitions(PRIVATE LOTTO_MEMMGR_RUNTIME)
