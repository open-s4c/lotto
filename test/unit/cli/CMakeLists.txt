file(GLOB SRCS *_test.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_executable(${TEST} ${SRC} engine_empty.c)
    target_compile_options(${TEST} PUBLIC -O0 -g)
    target_link_libraries(
        ${TEST}
        base_testing.o
        lotto-driver-engine.o
        cli_testing.o
        pthread
        sys_testing.o
        flags_prng_testing.o
        flags_sequencer_testing.o
        memmgr_runtime_libc.o
        memmgr_user_libc.o
        dice)
    add_test(NAME ${TEST} COMMAND ${TEST})
endforeach()
