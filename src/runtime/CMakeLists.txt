add_library(lotto_runtime SHARED
    runtime.c
    intercept_stub.c
    subs_pthread.c
    rsrc_deadlock_stub.c
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
    $<TARGET_OBJECTS:dice-dispatch.o>
    $<TARGET_OBJECTS:dice-self.o>
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
