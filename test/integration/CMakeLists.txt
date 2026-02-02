enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)
add_dependencies(tikl-test tikl-build)

add_subdirectory(basic)

# ------------------------------------------------------------------------------
# tikl integration tests
# ------------------------------------------------------------------------------

set(TSAN_0003-await-increment ON)
set(TSAN_0005-atomic-message-passing ON)
set(TSAN_0006-racy-message-passing ON)

# TODO: some sources are still failing
set(FAILING_SRCS
    debug-replay-end.c
    double_free.c
    enforce_address.c
    enforce_cat.c
    enforce_data.c
    enforce_pc.c
    explore_test.c
    filter_config.c
    filter_default.c
    impasse.c
    inflex_abc.c
    inflex_abc_nocli.c
    inflex_empty_test.c
    inflex_method_be.c
    inflex_method_le.c
    inflex_method_lp.c
    inflex_test.c
    loop_inflex_test.c
    notsan_mediator_null.c
    record_granularity_test_x86.c
    tsano_inflex_abc.c
    tsano_inflex_abc_stress.c
    wait.c)

file(GLOB ALL_SRCS *.c)
set(SRCS)
foreach(SRC ${ALL_SRCS})
    get_filename_component(SRC ${SRC} NAME)
    if(NOT "${SRC}" IN_LIST FAILING_SRCS)
        set(SRCS ${SRCS} ${SRC})
    endif()
endforeach()

add_custom_target(
    tikl-integration-test
    COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG} ${SRCS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(tikl-test tikl-integration-test)

foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    set(TARGET bin-${TEST})

    add_executable(${TARGET} ${SRC})
    set_target_properties(
        ${TARGET} PROPERTIES OUTPUT_NAME ${TEST} RUNTIME_OUTPUT_DIRECTORY
                                                 "${CMAKE_BINARY_DIR}/tikl")
    add_dependencies(tikl-test ${TARGET})

    if(${TSAN_${TARGET}})
        target_compile_options(${TARGET} PRIVATE -fsanitize=thread)
        target_link_options(${TARGET} PRIVATE -fsanitize=thread)
    endif()

    if("${LOTTO_TESTS_WITH_TSAN}")
        set(SANITIZE -fsanitize=thread)
    else()
        unset(SANITIZE)
    endif()
    target_compile_options(${TARGET} PUBLIC -O0 -g -DVATOMIC_BUILTINS)
    target_link_options(${TARGET} PUBLIC -rdynamic)
    target_link_libraries(${TARGET} PRIVATE vsync pthread)

    # This tikl target triggers only this specific test only. Use tikl-test to
    # run all tests.
    add_custom_target(
        tikl-${TEST}
        COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG} ${SRC}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(tikl-${TEST} ${TARGET} tikl-build lotto-cli)
endforeach()

# ##############################################################################
# Lit setup
# ##############################################################################

set(LIT_CFG lit/lit.cfg)
configure_file(lit.cfg.in ${LIT_CFG} @ONLY)

set(LIT_CFG_PATH ${CMAKE_CURRENT_BINARY_DIR}/${LIT_CFG})

set(LIT_WORKERS
    1
    CACHE STRING "Number of lit workers")
set(LIT_TIMEOUT
    300
    CACHE STRING "Lit timeout")

macro(forward_to_parent_scope VARS)
    foreach(VAR ${VARS})
        set(${VAR}
            ${${VAR}}
            PARENT_SCOPE)
    endforeach()
endmacro()

macro(add_lit_c_tests)
    set(oneValueArgs NIGHTLY_TESTS DAILY_SCRIPTS DAILY_DEPS NIGHTLY_SCRIPTS
                     NIGHTLY_DEPS)
    set(multiValueArgs SCRIPTS DEPS)
    set(PREFIX ADD_LIT_C_TESTS)
    cmake_parse_arguments(${PREFIX} "" "${oneValueArgs}" "${multiValueArgs}"
                          ${ARGN})
    file(GLOB SRCS *.c)
    foreach(SRC ${SRCS})
        get_filename_component(TEST ${SRC} NAME_WLE)

        set(COPIED_SRC "${CMAKE_CURRENT_BINARY_DIR}/lit/${TEST}.c")

        add_executable(${TEST} ${SRC})
        target_include_directories(${TEST}
                                   PRIVATE . ${PROJECT_SOURCE_DIR}/include)
        if("${LOTTO_TESTS_WITH_TSAN}" AND NOT "${TEST}" MATCHES "^notsan_")
            set(SANITIZE -fsanitize=thread)
        else()
            unset(SANITIZE)
        endif()
        target_compile_options(
            ${TEST} PUBLIC ${SANITIZE} -O0 -g -DVATOMIC_BUILTINS
                           ${COMPILE_OPTION_TRACE_PC})
        target_link_options(${TEST} PUBLIC -rdynamic)
        if(${TEST} MATCHES "^tsano_" OR NOT "${LOTTO_TSAN_RUNTIME}")
            set(TSAN tsano)
        else()
            set(TSAN tsan)
        endif()
        target_link_libraries(${TEST} PRIVATE vsync ${TSAN} ${LIB_TRACE_PC}
                                              pthread)

        if(${SRC} MATCHES "safe_stack")
            # Whitelist until error is fixed
            target_compile_options(${TEST} PRIVATE -Wno-shorten-64-to-32)
        endif()
        foreach(DEP ${${PREFIX}_DEPS})
            list(APPEND ${DEP} ${TEST})
        endforeach()
        foreach(SCRIPT ${${PREFIX}_SCRIPTS})
            list(APPEND ${SCRIPT} ${COPIED_SRC})
        endforeach()
        if(${TEST} IN_LIST ${${PREFIX}_NIGHTLY_TESTS})
            list(APPEND ${${PREFIX}_NIGHTLY_DEPS} ${TEST})
            list(APPEND ${${PREFIX}_NIGHTLY_SCRIPTS} ${COPIED_SRC})
        else()
            list(APPEND ${${PREFIX}_DAILY_DEPS} ${TEST})
            list(APPEND ${${PREFIX}_DAILY_SCRIPTS} ${COPIED_SRC})
        endif()
        add_lit_target(lit-${TEST} ${LIT_WORKERS} ${LIT_TIMEOUT} COPIED_SRC)
        add_dependencies(lit-${TEST} lit-prepare ${TEST})
    endforeach()
endmacro()

macro(add_lit_cpp_tests)
    set(oneValueArgs NIGHTLY_TESTS DAILY_SCRIPTS DAILY_DEPS NIGHTLY_SCRIPTS
                     NIGHTLY_DEPS)
    set(multiValueArgs SCRIPTS DEPS)
    set(PREFIX ADD_LIT_CPP_TESTS)
    cmake_parse_arguments(${PREFIX} "" "${oneValueArgs}" "${multiValueArgs}"
                          ${ARGN})
    file(GLOB SRCS *.cpp)
    foreach(SRC ${SRCS})
        get_filename_component(TEST ${SRC} NAME_WLE)

        set(COPIED_SRC "${CMAKE_CURRENT_BINARY_DIR}/lit/${TEST}.cpp")

        add_executable(${TEST} ${SRC})
        if("${LOTTO_TESTS_WITH_TSAN}")
            set(SANITIZE -fsanitize=thread)
        else()
            unset(SANITIZE)
        endif()
        target_compile_options(${TEST} PUBLIC -O0 -g -DVATOMIC_BUILTINS)
        target_link_options(${TEST} PUBLIC -rdynamic)
        if(${TEST} MATCHES "^tsano_" OR NOT "${LOTTO_TSAN_RUNTIME}")
            set(TSAN tsano)
        else()
            set(TSAN tsan)
        endif()
        target_link_libraries(${TEST} PRIVATE vsync pthread)
        foreach(DEP ${${PREFIX}_DEPS})
            list(APPEND ${DEP} ${TEST})
        endforeach()
        foreach(SCRIPT ${${PREFIX}_SCRIPTS})
            list(APPEND ${SCRIPT} ${COPIED_SRC})
        endforeach()
        if(${TEST} IN_LIST ${${PREFIX}_NIGHTLY_TESTS})
            list(APPEND ${${PREFIX}_NIGHTLY_DEPS} ${TEST})
            list(APPEND ${${PREFIX}_NIGHTLY_SCRIPTS} ${COPIED_SRC})
        else()
            list(APPEND ${${PREFIX}_DAILY_DEPS} ${TEST})
            list(APPEND ${${PREFIX}_DAILY_SCRIPTS} ${COPIED_SRC})
        endif()
        add_lit_target(lit-${TEST} ${LIT_WORKERS} ${LIT_TIMEOUT} COPIED_SRC)
        add_dependencies(lit-${TEST} lit-prepare ${TEST})
    endforeach()
endmacro()

macro(add_lit_sh_tests)
    set(oneValueArgs NIGHTLY_TESTS DAILY_SCRIPTS DAILY_DEPS NIGHTLY_SCRIPTS
                     NIGHTLY_DEPS)
    set(multiValueArgs SCRIPTS DEMO_SCRIPTS DEMO_DEPS TEST_SCRIPTS)
    set(PREFIX ADD_LIT_SH_TESTS)
    cmake_parse_arguments(${PREFIX} "" "${oneValueArgs}" "${multiValueArgs}"
                          ${ARGN})
    file(GLOB SRCS *.sh)
    foreach(SRC ${SRCS})
        get_filename_component(TEST ${SRC} NAME_WLE)
        string(REGEX REPLACE "(.*)(_demo.*)" \\1 DEMO ${TEST})
        set(COPIED_SRC "${CMAKE_CURRENT_BINARY_DIR}/lit/${TEST}.sh")

        add_lit_target(lit-${TEST} ${LIT_WORKERS} ${LIT_TIMEOUT} COPIED_SRC)
        add_dependencies(lit-${TEST} lit-prepare)
        if(TARGET ${DEMO})
            foreach(DEMO_SCRIPT ${${PREFIX}_DEMO_SCRIPTS})
                list(APPEND ${DEMO_SCRIPT} ${COPIED_SRC})
            endforeach()
            foreach(DEMO_DEP ${${PREFIX}_DEMO_DEPS})
                list(APPEND ${DEMO_DEP} ${DEMO})
            endforeach()
            add_dependencies(lit-${TEST} ${DEMO})
        else()
            foreach(TEST_SCRIPT ${${PREFIX}_TEST_SCRIPTS})
                list(APPEND ${TEST_SCRIPT} ${COPIED_SRC})
            endforeach()
            if(${TEST} IN_LIST ${${PREFIX}_NIGHTLY_TESTS})
                list(APPEND ${${PREFIX}_NIGHTLY_SCRIPTS} ${COPIED_SRC})
            else()
                list(APPEND ${${PREFIX}_DAILY_SCRIPTS} ${COPIED_SRC})
            endif()
        endif()
    endforeach()
endmacro()

set(LIT_WORKERS
    1
    CACHE STRING "Number of lit workers")
set(LIT_TIMEOUT
    300
    CACHE STRING "Lit timeout")

set(LIT_SRCS "")
set(LIT_DEPS "")
set(LIT_TESTS "")
set(LIT_TESTS_DEPS "")

# ##############################################################################
# Tests by pipeline
# ##############################################################################

set(LIT_NIGHTLY_TESTS)

set(LIT_DAILY_TIMEOUT
    180
    CACHE STRING "Lit daily timeout")
set(LIT_NIGHTLY_TIMEOUT
    600
    CACHE STRING "Lit nightly timeout")

set(LIT_DAILY_SRCS "")
set(LIT_DAILY_DEPS "")
set(LIT_NIGHTLY_SRCS "")
set(LIT_NIGHTLY_DEPS "")
set(LIT_DEMOS "")
set(LIT_DEMOS_DEPS "")

add_lit_prepare()
add_lit_c_tests(
    SCRIPTS
    LIT_SRCS
    LIT_TESTS
    DEPS
    LIT_DEPS
    LIT_TESTS_DEPS
    NIGHTLY_TESTS
    LIT_NIGHTLY_TESTS
    DAILY_SCRIPTS
    LIT_DAILY_SRCS
    DAILY_DEPS
    LIT_DAILY_DEPS
    NIGHTLY_SCRIPTS
    LIT_NIGHTLY_SRCS
    NIGHTLY_DEPS
    LIT_NIGHTLY_DEPS)
add_lit_cpp_tests(
    SCRIPTS
    LIT_SRCS
    LIT_TESTS
    DEPS
    LIT_DEPS
    LIT_TESTS_DEPS
    LIT_NIGHTLY_DEPS
    NIGHTLY_TESTS
    LIT_NIGHTLY_TESTS
    DAILY_SCRIPTS
    LIT_DAILY_SRCS
    DAILY_DEPS
    LIT_DAILY_DEPS
    NIGHTLY_SCRIPTS
    LIT_NIGHTLY_SRCS
    NIGHTLY_DEPS
    LIT_NIGHTLY_DEPS)
add_lit_sh_tests(
    SCRIPTS
    LIT_SRCS
    LIT_TESTS
    TEST_SCRIPTS
    LIT_SRCS
    LIT_TESTS
    DEMO_SCRIPTS
    LIT_SRCS
    LIT_DEMOS
    DEMO_DEPS
    LIT_DEMOS_DEPS
    NIGHTLY_TESTS
    LIT_NIGHTLY_TESTS
    DAILY_SCRIPTS
    LIT_DAILY_SRCS
    DAILY_DEPS
    LIT_DAILY_DEPS
    NIGHTLY_SCRIPTS
    LIT_NIGHTLY_SRCS
    NIGHTLY_DEPS
    LIT_NIGHTLY_DEPS)

add_lit_target(lit-nightly ${LIT_WORKERS} ${LIT_NIGHTLY_TIMEOUT}
               LIT_NIGHTLY_SRCS)
add_dependencies(lit-nightly lit-prepare ${LIT_NIGHTLY_DEPS})

add_lit_target(lit-daily ${LIT_WORKERS} ${LIT_DAILY_TIMEOUT} LIT_DAILY_SRCS)
add_dependencies(lit-daily lit-prepare ${LIT_DAILY_DEPS})

add_lit_target(lit-demos ${LIT_WORKERS} ${LIT_TIMEOUT} LIT_DEMOS)
add_dependencies(lit-demos lit-prepare ${LIT_DEMOS_DEPS})

# ##############################################################################
# General rules
# ##############################################################################

add_lit_target(lit ${LIT_WORKERS} ${LIT_TIMEOUT} LIT_SRCS)
add_dependencies(lit lit-prepare ${LIT_DEPS})
add_lit_target(lit-tests ${LIT_WORKERS} ${LIT_TIMEOUT} LIT_TESTS)
add_dependencies(lit-tests lit-prepare ${LIT_TESTS_DEPS})
