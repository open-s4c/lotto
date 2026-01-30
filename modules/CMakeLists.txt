enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)

# /* blocking handlers */                                                   #
# GEN_SLOT(BLOCKING)                                                        #
# GEN_SLOT(JOIN)                                                            #
# GEN_SLOT(MUTEX)                                                           #
# GEN_SLOT(EVEC)                                                            #
# GEN_SLOT(POLL)                                                            #
# GEN_SLOT(TIMEOUT)                                                         #
# GEN_SLOT(IMPASSE)                                                         # #
# /* hard filters */                                                        #
# GEN_SLOT(USER_FILTER)                                                     #
# GEN_SLOT(REGION_PREEMPTION)                                               #
# GEN_SLOT(REGION_FILTER)                                                   #
# GEN_SLOT(FILTERING)                                                       # #
# /* feature handlers */                                                    #
# GEN_SLOT(AVAILABLE)                                                       #
# GEN_SLOT(WATCHDOG)                                                        #
# GEN_SLOT(ICHPT)                                                           #
# GEN_SLOT(RACE)                                                            #
# GEN_SLOT(ATOMIC)                                                          #
# GEN_SLOT(YIELD)                                                           #
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

add_subdirectory(blocking)
# add_subdirectory(priority)
