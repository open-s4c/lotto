add_library(
    lotto_runtime SHARED
    # runtime files
    ../runtime/runtime.c
    ../runtime/interceptor.c
    ../runtime/mediator.c
    ../runtime/subs_adapter.c
    ../runtime/handler_config.c
    ../runtime/switcher.c
    ../runtime/thread_id.c
    ../runtime/await_stubs.c
    # engine files
    ../engine/engine.c
    ../engine/sequencer.c
    ../engine/recorder.c
    ../engine/dispatcher.c
    ../engine/prng_stub.c
    ../engine/handlers/handler_creation.c
    ../engine/handlers/handler_blocking.c
    ../engine/handlers/handler_mutex.c
    ../engine/handlers/handler_yield.c
    # base files
    ../base/marshable.c
    ../base/map.c
    ../base/tidmap.c
    ../base/tidset.c
    ../base/category.c
    ../base/slot.c
    ../base/plan.c
    ../base/record_granularity.c
    ../base/record.c
    ../base/string.c
    ../base/trace_file.c
    ../base/trace_impl.c
    # sys, brokers, and others
    ../sys/abort.c
    ../sys/libc_wrappers.c
    ../sys/stream_file.c
    ../sys/stream_impl.c
    ../brokers/statemgr.c
    ../brokers/pubsub_stub.c
    ../brokers/catmgr_stub.c
    ../states/sequencer.c
    ../states/handlers/handler_blocking.c
    ../states/handlers/handler_yield.c
    #
)
set_target_properties(lotto_runtime PROPERTIES OUTPUT_NAME lotto)

# Pull in dice object files from the subtree build.
target_sources(
    lotto_runtime
    PRIVATE $<TARGET_OBJECTS:dice.o>
            $<TARGET_OBJECTS:dice-self.o>
            $<TARGET_OBJECTS:dice-pthread_create.o>
            $<TARGET_OBJECTS:dice-pthread_mutex.o>
            $<TARGET_OBJECTS:dice-pthread_cond.o>
            $<TARGET_OBJECTS:dice-tsan.o>)

target_include_directories(
    lotto_runtime PUBLIC ${CMAKE_SOURCE_DIR}/include
                         $<BUILD_INTERFACE:${LOTTO_DICE_INCLUDE_DIR}>)

target_link_libraries(lotto_runtime PUBLIC dice)

install(TARGETS lotto_runtime)
