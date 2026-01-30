add_library(deadlock_component SHARED)
target_link_libraries(
    deadlock_component
    engine_component
    engine_engine_testing.o
    engine_sequencer_testing.o
    engine_recorder_testing.o
    base_testing.o
    sys_testing.o
    states_prng_testing.o
    states_sequencer_testing.o
    engine_dispatcher_testing.o
    brokers_testing.o
    engine_handler_creation_testing.o
    engine_handler_deadlock_testing.o
    states_handler_deadlock_testing.o
    memmgr_runtime.o
    memmgr_user.o
    dice.o
    dice-self.o)

add_racket_test(FILENAME deadlock_test.rkt ARG DT_DEADLOCK)
add_racket_test(FILENAME deadlock_test.rkt ARG DT_EXTRA_UNLOCK)
add_racket_test(FILENAME deadlock_test.rkt ARG DT_LOST_LOCK)
