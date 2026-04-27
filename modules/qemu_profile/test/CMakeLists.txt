set(TIKL_SRCS 0008-kernel-1core-profile.c)

foreach(SRC ${TIKL_SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_custom_target(
        tikl-${TEST}
        COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG}
                ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(tikl-${TEST} kernel-tests-aarch64 tikl-build lotto-cli)
    add_dependencies(tikl-module-${MODULE_NAME} tikl-${TEST})
endforeach()
