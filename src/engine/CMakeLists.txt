sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)
add_compile_definitions(LOGGER_PREFIX="engine")

set(MODULE_NAME engine)
set(MODULE_SLOT 8)
add_compile_definitions(DICE_MULTIFILE_MODULE DICE_MODULE_SLOT=${MODULE_SLOT})

set(ENGINE_RUNTIME_SRCS
    state.c
    statemgr.c
    prng.c
    engine.c
    plan.c
    sequencer.c
    recorder.c
    clock.c
    creation.c
    module.c)

set(ENGINE_DRIVER_SRCS flags.c state.c statemgr.c clock.c module.c)

message(STATUS "Module ${MODULE_SLOT} (runtime): ${MODULE_NAME}")
add_module_object(lotto-runtime-engine.o ${MODULE_SLOT} ${ENGINE_RUNTIME_SRCS})
target_link_libraries(lotto-runtime-engine.o PRIVATE lotto.h lotto-src.h dice.h)
target_compile_definitions(lotto-runtime-engine.o PRIVATE LOGGER_DISABLE)

message(STATUS "Module ${MODULE_SLOT} (runtime-dbg): ${MODULE_NAME}")
add_module_object(lotto-runtime-dbg-engine.o ${MODULE_SLOT}
                  ${ENGINE_RUNTIME_SRCS})
target_link_libraries(lotto-runtime-dbg-engine.o PRIVATE lotto.h lotto-src.h
                                                         dice.h)

message(STATUS "Module ${MODULE_SLOT} (driver) : ${MODULE_NAME}")
add_module_object(lotto-driver-engine.o ${MODULE_SLOT} ${ENGINE_DRIVER_SRCS})
target_link_libraries(lotto-driver-engine.o PRIVATE lotto.h lotto-src.h dice.h)

# Testing compatibility targets used by unit and racket test CMake files.
make_units(engine "")
target_sources(engine_sequencer_testing.o PRIVATE plan.c)
add_library(plan_testing.o OBJECT plan.c)
target_compile_definitions(plan_testing.o PRIVATE -DLOTTO_TEST)

add_library(brokers_testing.o OBJECT statemgr.c)
target_compile_definitions(brokers_testing.o PUBLIC -DLOTTO_TEST)

add_library(brokers_statemgr_testing.o OBJECT statemgr.c)
target_compile_definitions(brokers_statemgr_testing.o PUBLIC -DLOTTO_TEST)

add_library(statemgr_testing.o OBJECT statemgr.c)
target_compile_definitions(statemgr_testing.o PUBLIC -DLOTTO_TEST)
