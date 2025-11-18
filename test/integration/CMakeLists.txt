include("${CMAKE_SOURCE_DIR}/cmake/tikl.cmake")
lotto_ensure_tikl(TIKL_EXECUTABLE TIKL_CHECK_EXECUTABLE)

file(GLOB INTEGRATION_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*_test.c")
list(SORT INTEGRATION_TEST_SOURCES)

set(INTEGRATION_TEST_RELATIVE "")
foreach(src ${INTEGRATION_TEST_SOURCES})
    file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}" "${src}")
    list(APPEND INTEGRATION_TEST_RELATIVE "${rel}")
endforeach()

set(TSAN_mp_test ON)

set(INTEGRATION_TEST_TARGETS "")
foreach(src ${INTEGRATION_TEST_SOURCES})
    get_filename_component(test_name "${src}" NAME_WE)
    add_executable(${test_name} "${src}")
    target_include_directories(${test_name}
                               PRIVATE "${CMAKE_SOURCE_DIR}/include")
    target_link_libraries(${test_name} PRIVATE lotto_runtime pthread)
    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                  "${CMAKE_CURRENT_BINARY_DIR}")
    list(APPEND INTEGRATION_TEST_TARGETS ${test_name})
    if(${TSAN_${test_name}})
        target_compile_options(${test_name} PRIVATE -fsanitize=thread)
        target_link_options(${test_name} PRIVATE -fsanitize=thread)
    endif()
endforeach()

set(LOTTO_PATH "${CMAKE_SOURCE_DIR}/lotto")
configure_file(tikl.cfg.in "${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg" @ONLY)

add_custom_target(
    check-integration
    COMMAND
        ${CMAKE_COMMAND} -E env
        LOTTO_DICE_SCRIPT="${LOTTO_DICE_SCRIPTS_DIR}/dice" ${CMAKE_COMMAND} -E
        chdir "${CMAKE_SOURCE_DIR}" "${TIKL_EXECUTABLE}" -b
        "${CMAKE_BINARY_DIR}" -c "${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg"
        ${INTEGRATION_TEST_RELATIVE}
    COMMENT "Running tikl integration tests"
    USES_TERMINAL
    DEPENDS ${INTEGRATION_TEST_TARGETS} lotto_runtime tikl)
