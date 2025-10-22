include("${CMAKE_SOURCE_DIR}/cmake/tinl.cmake")
lotto_ensure_tinl(TINL_EXECUTABLE TINL_CHECK_EXECUTABLE)

file(GLOB INTEGRATION_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*_test.c")
list(SORT INTEGRATION_TEST_SOURCES)

set(INTEGRATION_TEST_TARGETS "")
foreach(src ${INTEGRATION_TEST_SOURCES})
    get_filename_component(test_name "${src}" NAME_WE)
    add_executable(${test_name} "${src}")
    target_include_directories(${test_name} PRIVATE "${CMAKE_SOURCE_DIR}/include")
    target_link_libraries(${test_name} PRIVATE pthread)
    set_target_properties(
        ${test_name}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    list(APPEND INTEGRATION_TEST_TARGETS ${test_name})
endforeach()

set(LOTTO_PATH "${CMAKE_SOURCE_DIR}/lotto")
configure_file(tinl.cfg.in "${CMAKE_CURRENT_BINARY_DIR}/tinl.cfg" @ONLY)

add_custom_target(
    check-integration
    COMMAND
        ${CMAKE_COMMAND} -E env
        LOTTO_DICE_SCRIPT="${LOTTO_DICE_SCRIPTS_DIR}/dice"
        "${TINL_EXECUTABLE}" -b "${CMAKE_CURRENT_BINARY_DIR}" -c
            "${CMAKE_CURRENT_BINARY_DIR}/tinl.cfg" ${INTEGRATION_TEST_SOURCES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Running tinl integration tests"
    USES_TERMINAL
    DEPENDS ${INTEGRATION_TEST_TARGETS} lotto_runtime tinl)
