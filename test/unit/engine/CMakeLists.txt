add_executable(sequencer_test sequencer_test.c)
target_link_libraries(
    sequencer_test
    PRIVATE engine_sequencer_testing.o
            base_testing.o
            engine_recorder_testing.o
            sys_testing.o
            engine_prng_testing.o
            engine_state_testing.o
            memmgr_runtime_libc.o
            memmgr_user_libc.o
            dice)
add_test(NAME sequencer_test COMMAND sequencer_test)

add_executable(statemgr_slot_test statemgr_slot_test.c)
target_link_libraries(statemgr_slot_test PRIVATE engine_statemgr_testing.o
    base_testing.o sys_testing.o memmgr_runtime_libc.o memmgr_user_libc.o dice)
add_test(NAME statemgr_slot_test COMMAND statemgr_slot_test)

add_executable(pos_reseed_test pos_reseed_test.c)
target_link_libraries(pos_reseed_test PRIVATE engine_recorder_testing.o
    base_testing.o sys_testing.o memmgr_runtime_libc.o memmgr_user_libc.o dice)
add_test(NAME pos_reseed_test COMMAND pos_reseed_test)
