add_library(lotto_uaf_runtime_engine SHARED uafcheck.c uaf_memmgr.c)
target_compile_definitions(lotto_uaf_runtime_engine
                           PRIVATE LOTTO_MEMMGR_RUNTIME)
target_link_libraries(lotto_uaf_runtime_engine PRIVATE engine)

add_library(lotto_uaf_user_engine SHARED uafcheck.c uaf_memmgr.c)
target_compile_definitions(lotto_uaf_user_engine PRIVATE LOTTO_MEMMGR_USER)
target_link_libraries(lotto_uaf_user_engine PRIVATE engine)

install(
    TARGETS lotto_uaf_runtime_engine lotto_uaf_user_engine
    DESTINATION DESTINATION
    "${LOTTO_PLUGIN_INSTALL_DIR}")
