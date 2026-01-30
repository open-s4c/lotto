add_binding(
    TARGET blocking-engine.rkt
    INCLUDE lotto/base/topic.h #
            lotto/engine/engine.h #
            lotto/engine/sequencer.h #
    EXCLUDE sys/types.h)

add_binding(
    TARGET recorder.rkt
    INCLUDE lotto/engine/recorder.h lotto/base/trace_flat.h
            lotto/sys/stream_file.h lotto/sys/stream_impl.h
    EXCLUDE sys/types.h)

add_library(mocked_recorder SHARED)
target_link_libraries(
    mocked_recorder
    engine_component
    engine_recorder_testing.o
    brokers_statemgr_testing.o
    base_testing.o
    sys_testing.o
    memmgr_runtime.o
    memmgr_user.o)

add_library(replay_component SHARED)
target_link_libraries(
    replay_component
    base_testing.o
    brokers_testing.o
    engine_creation_testing.o
    engine_dispatcher_testing.o
    engine_engine_testing.o
    engine_recorder_testing.o
    engine_sequencer_testing.o
    lotto-blocking.o
    memmgr_runtime.o
    memmgr_user.o
    states_prng_testing.o
    states_sequencer_testing.o
    sys_testing.o
    dice.o)

add_racket_test(FILENAME replay_test.rkt)
