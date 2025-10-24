set(QEMU_CMD ${CMAKE_CROSSCOMPILING_EMULATOR})

if(NOT "${QEMU_CMD}" STREQUAL "")
    set(LOTTO_OPTS -- ${QEMU_CMD})
endif()

set(LOTTO_CMD ${QEMU_CMD} ${PROJECT_BINARY_DIR}/lotto stress -r 10
              ${LOTTO_OPTS})

file(GLOB SRCS *.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)

    set(TARGET ${TEST}-native)
    add_executable(${TARGET} ${SRC})
    target_link_libraries(${TARGET} PRIVATE pthread)
    add_test(NAME ${TARGET} COMMAND ${QEMU_CMD} ./${TARGET})
    add_test(NAME ${TARGET}-stress COMMAND ${LOTTO_CMD} ./${TARGET})

    # Not compatible with ohos
    if(NOT DEFINED OHOS_SDK_ROOT_DIR)
        set(TARGET ${TEST}-tsan)
        add_executable(${TARGET} ${SRC})
        target_compile_options(${TARGET} PRIVATE -fsanitize=thread)
        target_link_libraries(${TARGET} PRIVATE pthread tsan)
        add_test(NAME ${TARGET}-stress COMMAND ${LOTTO_CMD} ./${TARGET})
    endif()
endforeach()
