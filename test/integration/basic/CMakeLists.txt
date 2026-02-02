set(TSAN_0003-await-increment ON)
set(TSAN_0005-atomic-message-passing ON)
set(TSAN_0006-racy-message-passing ON)

file(GLOB SRCS *.c)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(../tikl.cfg.in ${TIKL_CFG} @ONLY)

add_custom_target(
    tikl-basic-test
    COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG} ${SRCS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(tikl-test tikl-basic-test)

foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    set(TARGET bin-${TEST})

    add_executable(${TARGET} ${SRC})
    add_dependencies(tikl-basic-test ${TARGET})
    target_compile_options(${TARGET} PUBLIC -O0 -g -DVATOMIC_BUILTINS)
    target_link_options(${TARGET} PUBLIC -rdynamic)
    target_link_libraries(${TARGET} PRIVATE vsync pthread)
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${TEST})

    if(${TSAN_${TARGET}})
        target_compile_options(${TARGET} PRIVATE -fsanitize=thread)
        target_link_options(${TARGET} PRIVATE -fsanitize=thread)
    endif()

    add_custom_target(
        tikl-${TEST}
        COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG} ${SRC}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(tikl-${TEST} ${TARGET} tikl-build lotto-cli)
endforeach()
