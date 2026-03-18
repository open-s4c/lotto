set(SLOT 16) # we pick runtime

enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)

include_directories(blocking/include)
include_directories(ichpt/include)
include_directories(mutex/include)
include_directories(timeout/include)
include_directories(rusty/include)
include_directories(evec/include)
include_directories(order/include)
include_directories(region_preemption/include)
include_directories(deadlock/include)
include_directories(disable/include)
include_directories(rogue/include)
include_directories(time/include)
include_directories(task_velocity/include)
include_directories(yield/include)
include_directories(available/include)
include_directories(explore/include)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LOTTO_MODULE_BUILD_DIR}")

set(RUNTIME_MODULE_TYPE_rusty SHARED)
set(RUNTIME_MODULE_LTO_rusty OFF)

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
if(${LOTTO_RUST})
    add_module(rusty)
endif()

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
add_module(debug)
add_module(explore)
add_module(trace)
add_module(inflex)
if("${LOTTO_FRONTEND}" STREQUAL "QEMU")
    add_module(qemu)
endif()
add_module(fork)
