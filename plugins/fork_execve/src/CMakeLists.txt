add_library(lotto_fork_execve_engine SHARED intercept_fork.c state.c)

set_target_properties(
    lotto_fork_execve_engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                        "${LOTTO_PLUGIN_BUILD_DIR}")

target_include_directories(
    lotto_fork_execve_engine PRIVATE ../../../src/include ../../../include
                                     ../include)

target_link_libraries(lotto_fork_execve_engine PRIVATE engine)

add_library(lotto_fork_execve_cli SHARED flag.c state.c)

set_target_properties(
    lotto_fork_execve_cli PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                     "${LOTTO_PLUGIN_BUILD_DIR}")

target_include_directories(
    lotto_fork_execve_cli PRIVATE ../../../src/include ../../../include
                                  ../include)

install(
    TARGETS lotto_fork_execve_engine lotto_fork_execve_cli
    DESTINATION DESTINATION
    "${LOTTO_PLUGIN_INSTALL_DIR}")
