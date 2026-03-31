macro(add_runtime_module)
    set(RUNTIME_MODULE_TYPE OBJECT)
    if(DEFINED RUNTIME_MODULE_TYPE_${MODULE_NAME})
        set(RUNTIME_MODULE_TYPE ${RUNTIME_MODULE_TYPE_${MODULE_NAME}})
    endif()
    set(RUNTIME_MODULE_LTO ON)
    if(DEFINED RUNTIME_MODULE_LTO_${MODULE_NAME})
        set(RUNTIME_MODULE_LTO ${RUNTIME_MODULE_LTO_${MODULE_NAME}})
    endif()
    set(RUNTIME_MODULE_SOURCES ${ARGV})
    set(RUNTIME_MODULE_TARGET lotto-runtime-${MODULE_NAME})
    set(RUNTIME_DBG_MODULE_TARGET lotto-runtime-dbg-${MODULE_NAME})
    message(STATUS "Module ${MODULE_SLOT} (runtime): ${MODULE_NAME}")

    add_library(${RUNTIME_MODULE_TARGET} ${RUNTIME_MODULE_TYPE}
                                         ${RUNTIME_MODULE_SOURCES})
    add_library(${RUNTIME_DBG_MODULE_TARGET} ${RUNTIME_MODULE_TYPE}
                                             ${RUNTIME_MODULE_SOURCES})
    target_link_libraries(${RUNTIME_MODULE_TARGET} PRIVATE lotto.h dice.h)
    target_link_libraries(${RUNTIME_DBG_MODULE_TARGET} PRIVATE lotto.h dice.h)

    # enable LTO for this module?
    set(LTO FALSE)
    if(${LOTTO_LTO})
        if(${RUNTIME_MODULE_LTO})
            set(LTO TRUE)
        endif()
    endif()
    set_property(TARGET ${RUNTIME_MODULE_TARGET}
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION ${LTO})
    set_property(TARGET ${RUNTIME_DBG_MODULE_TARGET}
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION ${LTO})

    target_compile_definitions(
        ${RUNTIME_MODULE_TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                LOTTO_RUNTIME_MODULE=1 #
                LOTTO_DRIVER_MODULE=0 #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}" #
                LOGGER_DISABLE)
    target_compile_definitions(
        ${RUNTIME_DBG_MODULE_TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                LOTTO_RUNTIME_MODULE=1 #
                LOTTO_DRIVER_MODULE=0 #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}")
    if("${RUNTIME_MODULE_TYPE}" STREQUAL "SHARED")
        set_target_properties(
            ${RUNTIME_MODULE_TARGET}
            PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                 "${LOTTO_MODULE_BUILD_DIR}")
        set_target_properties(
            ${RUNTIME_DBG_MODULE_TARGET}
            PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                 "${LOTTO_MODULE_BUILD_DIR}")
        install(TARGETS ${RUNTIME_MODULE_TARGET}
                DESTINATION "${LOTTO_MODULE_INSTALL_DIR}")
        install(TARGETS ${RUNTIME_DBG_MODULE_TARGET}
                DESTINATION "${LOTTO_MODULE_INSTALL_DIR}")
        if(TARGET lotto-cli)
            add_dependencies(lotto-cli ${RUNTIME_MODULE_TARGET})
            add_dependencies(lotto-cli ${RUNTIME_DBG_MODULE_TARGET})
        endif()
    else()
        add_lotto_builtin(${RUNTIME_MODULE_TARGET} ${RUNTIME_DBG_MODULE_TARGET})
    endif()
endmacro()

function(runtime_module_link_libraries)
    target_link_libraries(${RUNTIME_MODULE_TARGET} ${ARGV})
    target_link_libraries(${RUNTIME_DBG_MODULE_TARGET} ${ARGV})
endfunction()

function(runtime_module_compile_definitions)
    target_compile_definitions(${RUNTIME_MODULE_TARGET} ${ARGV})
    target_compile_definitions(${RUNTIME_DBG_MODULE_TARGET} ${ARGV})
endfunction()

function(runtime_module_compile_options)
    target_compile_options(${RUNTIME_MODULE_TARGET} ${ARGV})
    target_compile_options(${RUNTIME_DBG_MODULE_TARGET} ${ARGV})
endfunction()

function(runtime_module_include_directories)
    target_include_directories(${RUNTIME_MODULE_TARGET} ${ARGV})
    target_include_directories(${RUNTIME_DBG_MODULE_TARGET} ${ARGV})
endfunction()

function(runtime_module_link_options)
    target_link_options(${RUNTIME_MODULE_TARGET} ${ARGV})
    target_link_options(${RUNTIME_DBG_MODULE_TARGET} ${ARGV})
endfunction()

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
    target_link_libraries(${DRIVER_MODULE_TARGET} PRIVATE lotto.h dice.h)
    target_compile_definitions(
        ${DRIVER_MODULE_TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                LOTTO_RUNTIME_MODULE=0 #
                LOTTO_DRIVER_MODULE=1 #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}")
    if("${DRIVER_MODULE_TYPE}" STREQUAL "SHARED")
        set_target_properties(
            ${DRIVER_MODULE_TARGET}
            PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                 "${LOTTO_MODULE_BUILD_DIR}")
        install(TARGETS ${DRIVER_MODULE_TARGET}
                DESTINATION "${LOTTO_MODULE_INSTALL_DIR}")
        if(TARGET lotto-cli)
            add_dependencies(lotto-cli ${DRIVER_MODULE_TARGET})
        endif()
    else()
        target_link_libraries(lotto-driver PRIVATE ${DRIVER_MODULE_TARGET})
    endif()
endmacro()

function(driver_module_link_libraries)
    target_link_libraries(${DRIVER_MODULE_TARGET} ${ARGV})
endfunction()

function(driver_module_compile_definitions)
    target_compile_definitions(${DRIVER_MODULE_TARGET} ${ARGV})
endfunction()

function(driver_module_compile_options)
    target_compile_options(${DRIVER_MODULE_TARGET} ${ARGV})
endfunction()

function(driver_module_include_directories)
    target_include_directories(${DRIVER_MODULE_TARGET} ${ARGV})
endfunction()

function(driver_module_link_options)
    target_link_options(${DRIVER_MODULE_TARGET} ${ARGV})
endfunction()

macro(new_module NAME)
    math(EXPR SLOT "${SLOT}+1")
    set(MODULE_NAME ${NAME})
    set(MODULE_SLOT ${SLOT})
endmacro()

macro(new_module_at NAME EXPLICIT_SLOT)
    set(MODULE_NAME ${NAME})
    set(MODULE_SLOT ${EXPLICIT_SLOT})
endmacro()

add_custom_target(tikl-modules)

function(add_tikl_module_target NAME)
    add_custom_target(tikl-module-${NAME})
    add_dependencies(tikl-test tikl-module-${NAME})
    add_dependencies(tikl-modules tikl-module-${NAME})
endfunction()

macro(add_module NAME)
    set(oneValueArgs SLOT)
    cmake_parse_arguments(ADD_MODULE "" "${oneValueArgs}" "" ${ARGN})
    option(LOTTO_MODULE_${NAME} "Build Lotto with module ${NAME}" ON)
    if(${LOTTO_MODULE_${NAME}})
        if(DEFINED ADD_MODULE_SLOT)
            new_module_at(${NAME} ${ADD_MODULE_SLOT})
        else()
            new_module(${NAME})
        endif()
        set(TIKL_MODULE_FEATURES "${TIKL_MODULE_FEATURES}\n-D module-${NAME}")
        add_tikl_module_target(${NAME})
        add_subdirectory(${NAME})
    endif()
endmacro()

macro(add_module_object TARGET SLOT)
    add_library(${TARGET} OBJECT ${ARGN})
    target_compile_definitions(${TARGET} PRIVATE DICE_MULTIFILE_MODULE
                                                 DICE_MODULE_SLOT=${SLOT})
endmacro()

function(add_module_tikl_test SRC)
    get_filename_component(TEST ${SRC} NAME_WLE)
    set(TARGET bin-${TEST})

    add_executable(${TARGET} ${SRC})
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${TEST})
    add_dependencies(tikl-module-${MODULE_NAME} ${TARGET})
    target_include_directories(
        ${TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/modules/qemu/include)

    if(NOT DEFINED TSAN_${TEST} OR "${TSAN_${TEST}}")
        target_compile_options(${TARGET} PRIVATE -fsanitize=thread)
        target_link_options(${TARGET} PRIVATE -fsanitize=thread)
        if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES
                                                  "Clang")
            target_compile_options(${TARGET} PRIVATE -shared-libsan)
            target_link_options(${TARGET} PRIVATE -shared-libsan)
        endif()
    endif()
    target_compile_options(${TARGET} PUBLIC -O0 -g -DVATOMIC_BUILTINS)
    target_link_options(${TARGET} PUBLIC -rdynamic)
    target_link_libraries(${TARGET} PRIVATE vsync pthread)

    add_custom_target(
        tikl-${TEST}
        COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG} ${SRC}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(tikl-${TEST} ${TARGET} tikl-build lotto-cli)
    add_dependencies(tikl-module-${MODULE_NAME} tikl-${TEST})
endfunction()
