add_binding(
    TARGET blocking-engine.rkt
    INCLUDE lotto/engine/events.h #
            lotto/runtime/events.h #
            lotto/driver/events.h #
            lotto/modules/deadlock/events.h #
            lotto/modules/enforce/events.h #
            lotto/modules/timeout/events.h #
            lotto/engine/engine.h #
            lotto/engine/sequencer.h #
    EXCLUDE sys/types.h)

add_binding(
    TARGET recorder.rkt
    INCLUDE lotto/engine/recorder.h lotto/base/trace_flat.h
            lotto/sys/stream_file.h lotto/sys/stream_impl.h
    EXCLUDE sys/types.h)

add_library(mocked_recorder SHARED)
if(${LOTTO_LTO})
    set_property(
        TARGET mocked_recorder PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()
target_link_libraries(
    mocked_recorder
    engine_component
    engine_recorder_testing.o
    brokers_statemgr_testing.o
    base_testing.o
    sys_testing.o
    memmgr_runtime_libc.o
    memmgr_user_libc.o)

add_library(replay_component SHARED)
if(${LOTTO_LTO})
    set_property(
        TARGET replay_component PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()
set_target_properties(
    replay_component PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                "${CMAKE_BINARY_DIR}/components")
target_link_libraries(
    replay_component
    base_testing.o
    brokers_statemgr_testing.o
    engine_creation_testing.o
    engine_dispatcher_testing.o
    engine_engine_testing.o
    engine_recorder_testing.o
    engine_sequencer_testing.o
    lotto-runtime-blocking
    memmgr_runtime_libc.o
    memmgr_user_libc.o
    engine_prng_testing.o
    engine_state_testing.o
    sys_testing.o
    dice.o)

add_racket_test(FILENAME replay_test.rkt)
