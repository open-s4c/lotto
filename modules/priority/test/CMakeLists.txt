file(GLOB SRCS *.c)

foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    set(TARGET bin-${TEST})
    add_executable(${TARGET} ${SRC})
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${TEST})
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
    add_custom_target(
        tikl-${TEST}
        COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG} ${SRC}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(tikl-${TEST} ${TARGET} tikl-build lotto-cli)

endforeach()
