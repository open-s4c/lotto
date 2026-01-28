add_executable(anytask_test anytask_test.c)
target_link_libraries(
    anytask_test
    sys_testing.o
    base_testing.o
    switcher_testing.o
    pthread
    memmgr_runtime.o
    memmgr_user.o)
add_test(NAME anytask_test COMMAND anytask_test)

add_executable(yield_continue_test yield_continue_test.c)
target_link_libraries(
    yield_continue_test
    sys_testing.o
    base_testing.o
    switcher_testing.o
    pthread
    memmgr_runtime.o
    memmgr_user.o)
add_test(NAME yield_continue_test COMMAND yield_continue_test)

add_executable(abort_test abort_test.c)
target_link_libraries(
    abort_test
    sys_testing.o
    base_testing.o
    switcher_testing.o
    pthread
    memmgr_runtime.o
    memmgr_user.o)
add_test(NAME abort_test COMMAND abort_test)

add_executable(slack_test slack_test.c)
target_link_libraries(
    slack_test
    sys_testing.o
    base_testing.o
    switcher_testing.o
    pthread
    memmgr_runtime.o
    memmgr_user.o)
add_test(NAME slack_test COMMAND slack_test)

file(GLOB SRCS *.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    target_compile_options(${TEST} PRIVATE -Wno-incompatible-pointer-types)
endforeach()
