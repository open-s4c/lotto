unset(CMAKE_C_CLANG_TIDY)
configure_file("lit.cfg.in" "lit/lit.cfg" @ONLY)

set(LIT_WORKERS 1)
set(LIT_TIMEOUT 120)

add_custom_target(
    lit-pattern-prepare
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/lit")

add_custom_target(
    lit-pattern COMMAND lit -v -j ${LIT_WORKERS} --timeout=${LIT_TIMEOUT}
                        "${CMAKE_CURRENT_BINARY_DIR}/lit")
add_dependencies(lit-pattern lit-pattern-prepare)

file(GLOB SRCS *.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_executable(${TEST} ${SRC})
    target_include_directories(${TEST} PRIVATE . ../../include)
    if("${LOTTO_TESTS_WITH_TSAN}")
        set(SANITIZE -fsanitize=thread)
    endif()
    target_compile_options(${TEST} PUBLIC ${SANITIZE} -O0 -g -DVATOMIC_BUILTINS
                                          ${COMPILE_OPTION_TRACE_PC})
    target_link_options(${TEST} PUBLIC -rdynamic)
    target_link_libraries(${TEST} PRIVATE vsync tsano pthread ${LIB_TRACE_PC})
    add_dependencies(lit-pattern ${TEST})
    add_custom_target(
        lit-pattern-${TEST}
        COMMAND lit -v -j ${LIT_WORKERS} --timeout=${LIT_TIMEOUT}
                "${CMAKE_CURRENT_BINARY_DIR}/lit/${TEST}.c")
    add_dependencies(lit-pattern-${TEST} lit-pattern-prepare ${TEST})
endforeach()

# ##############################################################################
# Tests by pipeline
# ##############################################################################

set(LIT_PATTERN_SLOW_TESTS
    RaRmw-5_RaRmw-5.LRaLRmwL-5_LRaLRmwL-5.55_89
    RW-10_RW-10.LRWL-10_LRWL-10.2047
    Rmw-10_Rmw-10.RmwL-10_RmwL-10.2047
    Rmw-10_Rmw-10.LRmwL-10_LRmwL-10.2047
    L-100_Rmw-10_Ra-1.L-100_LRmwL-10_RaD-1.0
    L-100_Rmw-10_Ra-1.L-100_LRmwL-10_RaD-1.7
    L-100_Rmw-10_Ra-1.L-100_LRmwL-10_RaD-1.10)

set(LIT_PATTERN_FAST_TESTS_SRC "")

file(GLOB SRCS *.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    if(NOT "${TEST}" IN_LIST LIT_PATTERN_SLOW_TESTS)
        list(APPEND LIT_PATTERN_FAST_TESTS_SRC
             "${CMAKE_CURRENT_BINARY_DIR}/lit/${TEST}.c")
    endif()
endforeach()

add_custom_target(
    lit-pattern-fast COMMAND lit -v -j ${LIT_WORKERS} --timeout=${LIT_TIMEOUT}
                             ${LIT_PATTERN_FAST_TESTS_SRC})
add_dependencies(lit-pattern-fast lit-pattern-prepare)
