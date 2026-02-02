enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)
set(SLOT 5)

macro(add_module NAME)
    math(EXPR SLOT "${SLOT}+1")
    add_subdirectory(${NAME})
    target_compile_definitions(lotto-${NAME}.o PRIVATE DICE_MODULE_SLOT=${SLOT}
                                                       DICE_MULTIFILE_MODULE)
    add_lotto_builtin(lotto-${NAME}.o)
endmacro()

include_directories(timeout/include)
include_directories(blocking/include)
include_directories(mutex/include)

# /* blocking handlers */                                                   #
add_module(blocking)
# GEN_SLOT(JOIN)                                                            #
add_module(mutex)
add_module(evec)
add_module(poll)
add_module(timeout)
add_module(impasse)
# /* hard filters */                                                        #
# GEN_SLOT(USER_FILTER)                                                     #
# GEN_SLOT(REGION_PREEMPTION)                                               #
# GEN_SLOT(REGION_FILTER)                                                   #
# GEN_SLOT(FILTERING)                                                       # #
# /* feature handlers */                                                    #
add_module(available)
add_module(watchdog)
# GEN_SLOT(ICHPT)                                                           #
# GEN_SLOT(RACE)                                                            #
# GEN_SLOT(ATOMIC)                                                          #
add_module(yield)
# GEN_SLOT(ADDRESS)                                                         #
# GEN_SLOT(DROP)                                                            #
# GEN_SLOT(TERMINATION)                                                     #
# GEN_SLOT(CAS)                                                             # #
# /* advanced handlers */                                                   #
# GEN_SLOT(CONSTRAINT_SATISFACTION)                                         #
# GEN_SLOT(RUSTY_ENGINE)                                                    #
# GEN_SLOT(RECONSTRUCT)                                                     #
# GEN_SLOT(PRIORITY)                                                        #
# GEN_SLOT(QEMU)                                                            #
# GEN_SLOT(CAPTURE_GROUP)                                                   #
# GEN_SLOT(TASK_VELOCITY)                                                   # #
# /* strategy handlers */                                                   #
# GEN_SLOT(POS)                                                             #
# GEN_SLOT(PCT)                                                             # #
# /* error detectors */                                                     #
# GEN_SLOT(DEADLOCK)                                                        #
# GEN_SLOT(ENFORCEMENT)                                                     # #
# /* other components */                                                    #
# GEN_SLOT(PRNG)                                                            #
# GEN_SLOT(CONFIG)                                                          #
# GEN_SLOT(CONTRACT)                                                        #
# GEN_SLOT(SEQUENCER)                                                       #
# GEN_SLOT(RECORDER)                                                        #
# GEN_SLOT(INACTIVITY_TIMEOUT)                                              #
# GEN_SLOT(CATMGR)                                                          # #
# GEN_SLOT(ANYSTALL)

# add_subdirectory(priority)
