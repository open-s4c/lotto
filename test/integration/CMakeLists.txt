include("${CMAKE_SOURCE_DIR}/cmake/tinl.cmake")
lotto_ensure_tinl(TINL_EXECUTABLE TINL_CHECK_EXECUTABLE)

add_executable(mutex_test mutex_test.c)
target_link_libraries(mutex_test PRIVATE pthread)
set_target_properties(
    mutex_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                           "${CMAKE_CURRENT_BINARY_DIR}")

set(LOTTO_PATH "${CMAKE_SOURCE_DIR}/lotto")
configure_file(tinl.cfg.in "${CMAKE_CURRENT_BINARY_DIR}/tinl.cfg" @ONLY)

add_custom_target(
    check-integration
    COMMAND "${TINL_EXECUTABLE}" -b "${CMAKE_CURRENT_BINARY_DIR}" -c
            "${CMAKE_CURRENT_BINARY_DIR}/tinl.cfg" mutex_test.c
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Running tinl integration tests"
    USES_TERMINAL
    DEPENDS mutex_test tinl_build)
