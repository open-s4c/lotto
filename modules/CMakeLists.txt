enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)

set(SLOT 5) # we pick runtime
macro(add_module NAME)
    math(EXPR SLOT "${SLOT}+1")
    add_subdirectory(${NAME})
    target_compile_definitions(lotto-${NAME}.o PRIVATE DICE_MODULE_SLOT=${SLOT}
                                                       DICE_MULTIFILE_MODULE)
    add_lotto_builtin(lotto-${NAME}.o)
endmacro()

include_directories(blocking/include)
include_directories(ichpt/include)
include_directories(mutex/include)
include_directories(timeout/include)

# blocking handlers
add_module(blocking)
# GEN_SLOT(JOIN)                                                            #
add_module(mutex)
add_module(evec)
add_module(poll)
add_module(timeout)
add_module(impasse)

# hard filters                                                                 #
# GEN_SLOT(USER_FILTER)                                                        #
add_module(region_preemption)
add_module(region_filter)
add_module(filtering)

# feature handlers
add_module(available)
add_module(watchdog)
add_module(ichpt)
add_module(race)
add_module(atomic)
add_module(yield)
add_module(termination)
# GEN_SLOT(CAS)                                                             # #

# advanced handlers                                                            #
# GEN_SLOT(RUSTY_ENGINE)                                                    #
add_module(priority)
# GEN_SLOT(QEMU)                                                            #
add_module(task_velocity)
add_module(order)
add_module(disable)

# scheduling strategy handlers
add_module(pos)
add_module(pct)

# error detectors
add_module(deadlock)
add_module(enforce)
add_module(inactivity_timeout)

# other components
add_module(time)
add_module(random)
add_module(tsan)

# GEN_SLOT(PRNG)                                                            #
# GEN_SLOT(CONFIG)                                                          #
# GEN_SLOT(CONTRACT)                                                        #
# GEN_SLOT(SEQUENCER)                                                       #
# GEN_SLOT(RECORDER)                                                        #

# GEN_SLOT(CATMGR)                                                          #
# GEN_SLOT(ANYSTALL)
