sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)

set(MODULE_NAME engine)
set(MODULE_SLOT 8)
add_compile_definitions(DICE_MULTIFILE_MODULE DICE_MODULE_SLOT=${MODULE_SLOT})

set(ENGINE_RUNTIME_SRCS
    state.c
    catmgr.c
    statemgr.c
    prng.c
    engine.c
    sequencer.c
    recorder.c
    dispatcher.c
    clock.c
    creation.c
    module.c)

set(ENGINE_DRIVER_SRCS
    flags.c
    state.c
    catmgr.c
    statemgr.c
    clock.c
    module.c)

message(STATUS "Module ${MODULE_SLOT} (runtime): ${MODULE_NAME}")
add_module_object(lotto-runtime-engine.o ${MODULE_SLOT} ${ENGINE_RUNTIME_SRCS})
target_link_libraries(lotto-runtime-engine.o PRIVATE lotto.h lotto-src.h dice.h)

message(STATUS "Module ${MODULE_SLOT} (driver) : ${MODULE_NAME}")
add_module_object(lotto-driver-engine.o ${MODULE_SLOT} ${ENGINE_DRIVER_SRCS})
target_link_libraries(lotto-driver-engine.o PRIVATE lotto.h lotto-src.h dice.h)

# Testing compatibility targets used by unit and racket test CMake files.
make_units(engine "")
target_sources(engine_state_testing.o PRIVATE catmgr.c)

add_library(brokers_testing.o OBJECT catmgr.c statemgr.c)
target_compile_definitions(brokers_testing.o PUBLIC -DLOTTO_TEST)

add_library(brokers_statemgr_testing.o OBJECT statemgr.c)
target_compile_definitions(brokers_statemgr_testing.o PUBLIC -DLOTTO_TEST)

add_library(statemgr_testing.o OBJECT statemgr.c)
target_compile_definitions(statemgr_testing.o PUBLIC -DLOTTO_TEST)
