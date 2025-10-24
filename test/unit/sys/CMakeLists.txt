file(GLOB SRCS *_test.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_executable(${TEST} ${SRC})
    target_link_libraries(${TEST} PUBLIC sys_testing.o base_testing.o
                                         memmgr_runtime.o memmgr_user.o)
endforeach()
