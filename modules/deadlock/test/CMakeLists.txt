if(${LOTTO_RACKET_TESTS})
    add_library(deadlock_component SHARED)
    set_target_properties(
        deadlock_component PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                      "${CMAKE_BINARY_DIR}/components")
    target_link_libraries(
        deadlock_component
        engine_component
        engine_creation_testing.o
        engine_engine_testing.o
        engine_sequencer_testing.o
        engine_recorder_testing.o
        base_testing.o
        sys_testing.o
        engine_prng_testing.o
        engine_state_testing.o
        engine_dispatcher_testing.o
        brokers_statemgr_testing.o
        lotto-runtime-deadlock
        memmgr_runtime_libc.o
        memmgr_user_libc.o
        dice.o
        dice-self.o)

endif()

file(GLOB SRCS *.c)

foreach(SRC ${SRCS})
    add_module_tikl_test(${SRC})
endforeach()
