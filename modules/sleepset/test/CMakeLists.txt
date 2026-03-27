include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(sleepset_test sleepset_test.c)
target_link_libraries(
    sleepset_test
    PRIVATE lotto-runtime-${MODULE_NAME}
            sys_testing.o
            base_testing.o
            statemgr_testing.o
            memmgr_runtime_libc.o
            memmgr_user_libc.o
            dice)
target_link_options(sleepset_test PUBLIC -rdynamic)
add_test(NAME sleepset_test COMMAND sleepset_test)
