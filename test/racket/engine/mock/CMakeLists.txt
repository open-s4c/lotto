gen_pubsub_headers("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
include_directories(${CMAKE_BINARY_DIR}/generated)

# prepare base component and mocked parts
add_mock(
    TARGET engine_mock.c
    INCLUDE lotto/engine/engine.h #
            lotto/base/context.h #
            lotto/brokers/pubsub.h #
            lotto/brokers/statemgr.h #
            lotto/engine/sequencer.h #
            lotto/engine/prng.h #
            lotto/engine/dispatcher.h #
            lotto/engine/recorder.h #
    EXCLUDE io.h
            logger.h
            FLAGS
            -I
            ${PROJECT_BINARY_DIR}/include
            -I
            ${PROJECT_BINARY_DIR}/generated)

add_library(engine_component SHARED engine_mock.c)
target_link_libraries(
    engine_component
    engine_engine_testing.o
    base_testing.o
    sys_testing.o
    brokers_testing.o
    memmgr_runtime.o
    memmgr_user.o)

add_binding(
    TARGET engine.rkt
    INCLUDE lotto/base/topic.h #
            lotto/engine/engine.h #
            lotto/engine/sequencer.h #
    EXCLUDE sys/types.h)

add_binding(
    TARGET sequencer.rkt
    INCLUDE lotto/engine/sequencer.h lotto/base/trace_flat.h
            lotto/engine/recorder.h lotto/engine/dispatcher.h
    EXCLUDE sys/types.h)
add_library(mocked_sequencer SHARED)
target_link_libraries(
    mocked_sequencer
    engine_component
    engine_sequencer_testing.o
    engine_recorder_testing.o
    brokers_statemgr_testing.o
    base_testing.o
    sys_testing.o
    memmgr_runtime.o
    memmgr_user.o)
