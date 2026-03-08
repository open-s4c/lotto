set(SLOT 16) # we pick runtime
set(RUNTIME_MODULE_TYPE_uafcheck SHARED)
set(RUNTIME_MODULE_TYPE_leakcheck SHARED)
set(RUNTIME_MODULE_TYPE_user_mempool SHARED)

enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)

include_directories(blocking/include)
include_directories(ichpt/include)
include_directories(mutex/include)
include_directories(timeout/include)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LOTTO_MODULE_BUILD_DIR}")

# blocking handlers
add_module(blocking)
add_module(join)
add_module(mutex)
add_module(rwlock)
add_module(evec)
add_module(poll)
add_module(timeout)
add_module(impasse)
add_module(cxa)

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

# advanced handlers                                                            #
add_module(priority)
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
if("${LOTTO_DRUM}")
    add_module(drum)
endif()
add_module(explore)
if("${LOTTO_FRONTEND}" STREQUAL "QEMU")
    add_module(qemu)
endif()
add_module(fork)
add_module(uafcheck)
add_module(leakcheck)
add_module(user_mempool)
if(${LOTTO_RUST})
    add_module(rusty)
endif()

# GEN_SLOT(PRNG)                                                            #
# GEN_SLOT(CONFIG)                                                          #
# GEN_SLOT(CONTRACT)                                                        #
# GEN_SLOT(SEQUENCER)                                                       #
# GEN_SLOT(RECORDER)                                                        #

# GEN_SLOT(CATMGR)                                                          #
# GEN_SLOT(ANYSTALL)
