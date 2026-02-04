set(SLOT 9) # we pick runtime
set(RUNTIME_MODULE_TYPE_uafcheck SHARED)
set(RUNTIME_MODULE_TYPE_leakcheck SHARED)
set(RUNTIME_MODULE_TYPE_user_mempool SHARED)

enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)

macro(add_runtime_module)
    set(RUNTIME_MODULE_TYPE OBJECT)
    if(DEFINED RUNTIME_MODULE_TYPE_${MODULE_NAME})
        set(RUNTIME_MODULE_TYPE ${RUNTIME_MODULE_TYPE_${MODULE_NAME}})
    endif()
    set(RUNTIME_MODULE_SOURCES ${ARGV})
    set(RUNTIME_MODULE_TARGET lotto-${MODULE_NAME})
    message(STATUS "Module ${MODULE_NAME} (runtime): ${RUNTIME_MODULE_SOURCES}")

    add_library(${RUNTIME_MODULE_TARGET} ${RUNTIME_MODULE_TYPE}
                                         ${RUNTIME_MODULE_SOURCES})
    target_link_libraries(${RUNTIME_MODULE_TARGET} PRIVATE lotto.h lotto-src.h
                                                           dice.h)
    target_compile_definitions(
        ${RUNTIME_MODULE_TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}")
    if("${RUNTIME_MODULE_TYPE}" STREQUAL "SHARED")
        set_target_properties(
            ${RUNTIME_MODULE_TARGET}
            PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                 "${LOTTO_MODULE_BUILD_DIR}")
    else()
        add_lotto_builtin(${RUNTIME_MODULE_TARGET})
    endif()
endmacro()

macro(add_driver_module)
    set(DRIVER_MODULE_TYPE SHARED)
    if(DEFINED DRIVER_MODULE_TYPE_${MODULE_NAME})
        set(DRIVER_MODULE_TYPE ${DRIVER_MODULE_TYPE_${MODULE_NAME}})
    endif()
    set(DRIVER_MODULE_SOURCES ${ARGV})
    set(DRIVER_MODULE_TARGET lotto-driver-${MODULE_NAME})
    message(STATUS "Module ${MODULE_NAME} (driver): ${DRIVER_MODULE_SOURCES}")

    add_library(${DRIVER_MODULE_TARGET} ${DRIVER_MODULE_TYPE}
                                        ${DRIVER_MODULE_SOURCES})
    target_link_libraries(${DRIVER_MODULE_TARGET} PRIVATE lotto.h lotto-src.h
                                                          dice.h)
    target_compile_definitions(
        ${DRIVER_MODULE_TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}")
    if("${DRIVER_MODULE_TYPE}" STREQUAL "SHARED")
        set_target_properties(
            ${DRIVER_MODULE_TARGET}
            PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                 "${LOTTO_MODULE_BUILD_DIR}")
    else()
        add_lotto_builtin(${DRIVER_MODULE_TARGET})
    endif()
    if("${DRIVER_MODULE_TYPE}" STREQUAL "SHARED")
        set_target_properties(
            ${DRIVER_MODULE_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                               "${LOTTO_MODULE_BUILD_DIR}")
        target_link_libraries(lotto-driver PRIVATE ${DRIVER_MODULE_TARGET})
    else()
        target_link_libraries(lotto-driver PRIVATE ${DRIVER_MODULE_TARGET})
    endif()
endmacro()

macro(add_module NAME)
    math(EXPR SLOT "${SLOT}+1")
    set(MODULE_NAME ${NAME})
    set(MODULE_SLOT ${SLOT})
    add_subdirectory(${NAME})
endmacro()

include_directories(blocking/include)
include_directories(ichpt/include)
include_directories(mutex/include)
include_directories(timeout/include)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LOTTO_MODULE_BUILD_DIR}")

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
