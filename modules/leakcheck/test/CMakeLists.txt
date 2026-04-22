if(TARGET engine_component)
    file(GLOB SRCS *_test.c)
    foreach(SRC ${SRCS})
        get_filename_component(TARGET ${SRC} NAME_WLE)
        add_executable(${TARGET} ${SRC})
        target_link_libraries(
            ${TARGET}
            base_testing.o
            sys_testing.o
            engine_component
            memmgr_runtime_libc.o
            memmgr_user_libc.o
            lotto-runtime-leakcheck-memmgr_user)
        add_test(NAME ${TARGET} COMMAND ${TARGET})
    endforeach()
endif()
