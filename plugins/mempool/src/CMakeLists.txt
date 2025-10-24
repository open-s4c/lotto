include_directories(../../../src/include ../../../include ../include)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LOTTO_PLUGIN_BUILD_DIR}")

add_library(lotto_mempool_runtime_engine SHARED mempool_memmgr.c)
target_compile_definitions(lotto_mempool_runtime_engine
                           PRIVATE LOTTO_MEMMGR_RUNTIME)
target_link_libraries(lotto_mempool_runtime_engine PRIVATE engine)

add_library(lotto_mempool_user_engine SHARED mempool_memmgr.c)
target_compile_definitions(lotto_mempool_user_engine PRIVATE LOTTO_MEMMGR_USER)
target_link_libraries(lotto_mempool_user_engine PRIVATE engine)

add_library(lotto_mempool_runtime_cli SHARED mempool_flag.c)
target_compile_definitions(lotto_mempool_runtime_cli
                           PRIVATE LOTTO_MEMMGR_RUNTIME)

add_library(lotto_mempool_user_cli SHARED mempool_flag.c)
target_compile_definitions(lotto_mempool_user_cli PRIVATE LOTTO_MEMMGR_USER)

install(
    TARGETS lotto_mempool_runtime_engine lotto_mempool_user_engine
            lotto_mempool_runtime_cli lotto_mempool_user_cli
    DESTINATION DESTINATION
    "${LOTTO_PLUGIN_INSTALL_DIR}")
