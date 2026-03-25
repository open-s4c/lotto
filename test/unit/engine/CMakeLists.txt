add_executable(sequencer_test sequencer_test.c)
target_link_libraries(
    sequencer_test
    PRIVATE engine_sequencer_testing.o
            base_testing.o
            engine_clock_testing.o
            engine_recorder_testing.o
            sys_testing.o
            engine_prng_testing.o
            engine_state_testing.o
            memmgr_runtime_libc.o
            memmgr_user_libc.o
            dice)
add_test(NAME sequencer_test COMMAND sequencer_test)
