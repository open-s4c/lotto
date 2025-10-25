add_library(lotto_runtime SHARED
    runtime.c
    interceptor.c
    mediator.c
    #intercept_pthread.c
    #intercept_posix.c
    #intercept_order.c
    #intercept_mutex.c
    #intercept_malloc.c
    #intercept_evec.c
    #intercept_rsrc_deadlock.c
    #intercept_tsan.c
    subs_adapter.c
    handler_config.c
    switcher.c
    thread_id.c
    ../engine/engine.c
    ../engine/sequencer.c
    ../engine/recorder.c
    ../engine/dispatcher.c
    ../engine/prng_stub.c
    await_stubs.c
    ../base/marshable.c
    ../base/map.c
    ../base/tidmap.c
    ../base/tidset.c
    ../base/category.c
    ../base/slot.c
    ../base/plan.c
    ../base/record_granularity.c
    ../base/string.c
    ../sys/abort.c
    ../sys/libc_wrappers.c
    ../brokers/statemgr_stub.c
    ../brokers/pubsub_stub.c
    ../brokers/catmgr_stub.c
    ../states/sequencer.c
)
set_target_properties(lotto_runtime PROPERTIES OUTPUT_NAME lotto)

# Pull in dice object files from the subtree build.
target_sources(lotto_runtime PRIVATE
    $<TARGET_OBJECTS:dice.o>
    $<TARGET_OBJECTS:dice-self.o>
    $<TARGET_OBJECTS:dice-pthread_create.o>
    $<TARGET_OBJECTS:dice-pthread_mutex.o>
    $<TARGET_OBJECTS:dice-pthread_cond.o>
    $<TARGET_OBJECTS:dice-tsan.o>
)

target_include_directories(lotto_runtime
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        $<BUILD_INTERFACE:${LOTTO_DICE_INCLUDE_DIR}>
)

target_link_libraries(lotto_runtime
    PUBLIC
        dice
)

install(TARGETS lotto_runtime)
