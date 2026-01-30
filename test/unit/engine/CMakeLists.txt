add_subdirectory(util)

add_executable(sequencer_test sequencer_test.c)
target_link_libraries(
    sequencer_test
    PRIVATE engine_sequencer_testing.o
            base_testing.o
            # brokers_pubsub_testing.o
            engine_clock_testing.o
            engine_recorder_testing.o
            sys_testing.o
            states_prng_testing.o
            states_sequencer_testing.o
            memmgr_runtime.o
            memmgr_user.o
            dice)
add_test(NAME sequencer_test COMMAND sequencer_test)
