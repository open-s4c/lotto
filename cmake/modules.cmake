macro(add_runtime_module)
    set(RUNTIME_MODULE_TYPE OBJECT)
    if(DEFINED RUNTIME_MODULE_TYPE_${MODULE_NAME})
        set(RUNTIME_MODULE_TYPE ${RUNTIME_MODULE_TYPE_${MODULE_NAME}})
    endif()
    set(RUNTIME_MODULE_SOURCES ${ARGV})
    set(RUNTIME_MODULE_TARGET lotto-runtime-${MODULE_NAME})
    message(STATUS "Module ${MODULE_SLOT} (runtime): ${MODULE_NAME}")

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
    message(STATUS "Module ${MODULE_SLOT} (driver) : ${MODULE_NAME}")

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
        target_link_libraries(lotto-driver PRIVATE ${DRIVER_MODULE_TARGET})
    endif()
endmacro()

macro(new_module NAME)
    math(EXPR SLOT "${SLOT}+1")
    set(MODULE_NAME ${NAME})
    set(MODULE_SLOT ${SLOT})
endmacro()

macro(add_module NAME)
    new_module(${NAME})
    add_subdirectory(${NAME})
endmacro()

macro(add_module_object TARGET SLOT)
    add_library(${TARGET} OBJECT ${ARGN})
    target_compile_definitions(
        ${TARGET} PRIVATE DICE_MULTIFILE_MODULE DICE_MODULE_SLOT=${SLOT})
endmacro()
