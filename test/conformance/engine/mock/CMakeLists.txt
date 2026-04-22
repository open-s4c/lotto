include_directories(${CMAKE_BINARY_DIR}/generated)

add_library(engine_component SHARED engine_mock.c)
target_link_libraries(
    engine_component
    engine_engine_testing.o
    base_testing.o
    sys_testing.o
    brokers_testing.o
    memmgr_runtime_libc.o
    memmgr_user_libc.o)
add_library(mocked_sequencer SHARED)
target_link_libraries(
    mocked_sequencer
    engine_component
    engine_sequencer_testing.o
    engine_recorder_testing.o
    brokers_statemgr_testing.o
    base_testing.o
    sys_testing.o
    memmgr_runtime_libc.o
    memmgr_user_libc.o)
