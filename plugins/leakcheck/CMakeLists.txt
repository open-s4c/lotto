include_directories(../../src/include ../../include)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LOTTO_PLUGIN_BUILD_DIR}")

add_library(lotto_leakcheck_runtime_engine SHARED leakcheck.c)
target_compile_definitions(lotto_leakcheck_runtime_engine
                           PRIVATE LOTTO_MEMMGR_RUNTIME)
target_link_libraries(lotto_leakcheck_runtime_engine PRIVATE engine)

add_library(lotto_leakcheck_user_engine SHARED leakcheck.c)
target_compile_definitions(lotto_leakcheck_user_engine
                           PRIVATE LOTTO_MEMMGR_USER)
target_link_libraries(lotto_leakcheck_user_engine PRIVATE engine)

install(
    TARGETS lotto_leakcheck_runtime_engine lotto_leakcheck_user_engine
    DESTINATION DESTINATION
    "${LOTTO_PLUGIN_INSTALL_DIR}")
