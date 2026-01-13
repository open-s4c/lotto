file(GLOB SRCS *_test.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_executable(${TEST} ${SRC} engine_empty.c)
    target_compile_options(${TEST} PUBLIC -O0 -g)
    target_link_libraries(
        ${TEST}
        base_testing.o
        brokers_testing.o
        cli_testing.o
        pthread
        sys_testing.o
        states_prng_testing.o
        states_sequencer_testing.o
        flags_prng_testing.o
        flags_sequencer_testing.o
        memmgr_runtime.o
        memmgr_user.o
        dice)
    add_test(NAME ${TEST} COMMAND ${TEST})
endforeach()
