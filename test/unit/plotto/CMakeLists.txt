add_subdirectory(switcher)

if(UNIX)
    set(EMPTY_FILE "/dev/null")
else()
    set(EMPTY_FILE "${CMAKE_CURRENT_BINARY_DIR}/empty")
    file(WRITE ${EMPTY_FILE} )
endif()

file(GLOB SRCS *.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_library(${TEST}.o OBJECT ${SRC})
    add_executable(${TEST} ${EMPTY_FILE})
    target_link_libraries(
        ${TEST}
        ${TEST}.o
        sys_unreal_testing.o
        engine_clock_testing.o
        base_testing.o
        brokers_testing.o
        pthread
        interceptor_testing.o
        posix_interceptors.o
        memmgr_runtime.o
        memmgr_user.o)
    # add_test(NAME ${TEST} COMMAND ${TEST})
endforeach()
