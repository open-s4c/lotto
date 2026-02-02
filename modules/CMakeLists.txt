enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)

set(SLOT 9) # we pick runtime

function(add_runtime_module)
    set(MODULE_RUNTIME_SOURCES ${ARGV})
    message(STATUS "Module ${MODULE_NAME} (runtime): ${MODULE_RUNTIME_SOURCES}")
    set(LIB_TYPE OBJECT)
    set(TARGET lotto-${MODULE_NAME})

    add_library(${TARGET} ${LIB_TYPE} ${MODULE_RUNTIME_SOURCES})
    target_link_libraries(${TARGET} PRIVATE lotto.h lotto-src.h dice.h)
    target_compile_definitions(
        ${TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}")
    if("${LIB_TYPE}" STREQUAL "SHARED")
        set_target_properties(
            ${TARGET} PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                           "${LOTTO_PLUGIN_BUILD_DIR}")
    else()
        add_lotto_builtin(${TARGET})
    endif()
endfunction()

macro(add_driver_module)
    set(MODULE_DRIVER_SOURCES ${ARGN})
    message(STATUS "Module ${MODULE_NAME} (driver):  ${MODULE_DRIVER_SOURCES}")
    set(LIB_TYPE OBJECT)
    set(TARGET lotto-driver-${MODULE_NAME})
    add_library(${TARGET} ${LIB_TYPE} ${MODULE_DRIVER_SOURCES})
    target_link_libraries(${TARGET} PRIVATE lotto.h lotto-src.h dice.h)
    target_compile_definitions(
        ${TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}")
    if("${LIB_TYPE}" STREQUAL "SHARED")
        set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                   "${LOTTO_PLUGIN_BUILD_DIR}")
        target_link_libraries(lotto-driver PRIVATE ${TARGET})
    else()
        target_link_libraries(lotto-driver PRIVATE ${TARGET})
    endif()
endmacro()

macro(add_module NAME)
    math(EXPR SLOT "${SLOT}+1")
    unset(MODULE_RUNTIME_SOURCES)
    unset(MODULE_DRIVER_SOURCES)
    set(MODULE_NAME ${NAME})
    set(MODULE_SLOT ${SLOT})
    add_subdirectory(${NAME})
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
add_module(qemu)

# GEN_SLOT(PRNG)                                                            #
# GEN_SLOT(CONFIG)                                                          #
# GEN_SLOT(CONTRACT)                                                        #
# GEN_SLOT(SEQUENCER)                                                       #
# GEN_SLOT(RECORDER)                                                        #

# GEN_SLOT(CATMGR)                                                          #
# GEN_SLOT(ANYSTALL)
