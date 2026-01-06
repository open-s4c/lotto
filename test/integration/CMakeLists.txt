include("${CMAKE_SOURCE_DIR}/cmake/tikl.cmake")
lotto_ensure_tikl(TIKL_EXECUTABLE TIKL_CHECK_EXECUTABLE)

set(LOTTO_PATH "${CMAKE_SOURCE_DIR}/lotto")
configure_file(tikl.cfg.in "${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg" @ONLY)

set(TSAN_mp_test ON)

file(GLOB SRCS "000*.c")
list(SORT SRCS)
message(STATUS "integration tests: ${SRCS}")
foreach(SRC ${SRCS})
    get_filename_component(TARGET ${SRC} NAME_WE)
    add_executable(${TARGET} ${SRC})

    target_link_libraries(${TARGET} PRIVATE pthread)
    list(APPEND INTEGRATION_TEST_TARGETS ${TARGET})
    if(${TSAN_${TARGET}})
        target_compile_options(${TARGET} PRIVATE -fsanitize=thread)
        target_link_options(${TARGET} PRIVATE -fsanitize=thread)
    endif()

    add_test(
        NAME integration-${TARGET}
        COMMAND
            "${TIKL_EXECUTABLE}" #
            -b "${CMAKE_CURRENT_BINARY_DIR}" #
            -c "${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg" #
            ${SRC})

    set_tests_properties(integration-${TARGET}
                         PROPERTIES DEPENDS "${TARGET};lotto_runtime")
endforeach()
