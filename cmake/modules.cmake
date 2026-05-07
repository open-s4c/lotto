function(ensure_module_type VARNAME)
    set(_var ${VARNAME})
    set(_val ${${VARNAME}})
    if(NOT "${_val}" IN_LIST LOTTO_MODULE_TYPE_LIST)
        message(FATAL_ERROR "Invalid type ${_var}: `${_val}`. "
                            "Use one of these: `${LOTTO_MODULE_TYPE_LIST}`")
    endif()
endfunction()

function(ensure_module_name_prefix NAME)
    if(NOT DEFINED MODULE_ROOT_NAME)
        return()
    endif()

    string(REGEX MATCH "^${MODULE_ROOT_NAME}($|-.*)" _is_valid_module_name
                 "${NAME}")
    if(NOT _is_valid_module_name)
        message(
            FATAL_ERROR
                "Invalid module artifact name `${NAME}`. "
                "Expected `${MODULE_ROOT_NAME}` or `${MODULE_ROOT_NAME}-...` "
                "(producing lotto-(driver|runtime)-(dbg-)?${MODULE_ROOT_NAME}(-.*)?.${CMAKE_SHARED_LIBRARY_SUFFIX}."
        )
    endif()
endfunction()

set(LOTTO_DEFAULT_RUNTIME_MODULE_TYPE
    BUILTIN
    CACHE STRING "Default type of runtime modules")
set_property(CACHE LOTTO_DEFAULT_RUNTIME_MODULE_TYPE
             PROPERTY STRINGS ${LOTTO_MODULE_TYPE_LIST})
ensure_module_type(LOTTO_DEFAULT_RUNTIME_MODULE_TYPE)
set(LOTTO_DEFAULT_DRIVER_MODULE_TYPE
    PLUGIN
    CACHE STRING "Default type of driver modules")
set_property(CACHE LOTTO_DEFAULT_DRIVER_MODULE_TYPE
             PROPERTY STRINGS ${LOTTO_MODULE_TYPE_LIST})
ensure_module_type(LOTTO_DEFAULT_DRIVER_MODULE_TYPE)

function(lotto_module_metadata_add NAME SLOT COMPONENT TYPE)
    string(TOLOWER "${TYPE}" _type_lower)
    set_property(
        GLOBAL APPEND PROPERTY LOTTO_MODULE_METADATA
                               "${SLOT}|${NAME}|${COMPONENT}|${_type_lower}")
endfunction()

macro(add_runtime_module)
    if(NOT DEFINED MODULE_NAME)
        message(FATAL_ERROR "Undefined module name")
    endif()
    ensure_module_type(RUNTIME_MODULE_TYPE_${MODULE_NAME})
    set(RUNTIME_MODULE_TYPE ${RUNTIME_MODULE_TYPE_${MODULE_NAME}})

    if(${RUNTIME_MODULE_TYPE} STREQUAL BUILTIN)
        set(_lib_type "OBJECT")
    else()
        set(_lib_type "SHARED")
    endif()

    set(RUNTIME_MODULE_LTO ON)
    if(DEFINED RUNTIME_MODULE_LTO_${MODULE_NAME})
        set(RUNTIME_MODULE_LTO ${RUNTIME_MODULE_LTO_${MODULE_NAME}})
    endif()
    set(RUNTIME_MODULE_SOURCES ${ARGV})
    set(RUNTIME_MODULE_TARGET lotto-runtime-${MODULE_NAME})
    set(RUNTIME_DBG_MODULE_TARGET lotto-runtime-dbg-${MODULE_NAME})
    message(STATUS "Module ${MODULE_SLOT} (runtime): ${MODULE_NAME}")
    lotto_module_metadata_add(${MODULE_NAME} ${MODULE_SLOT} runtime
                              ${RUNTIME_MODULE_TYPE})

    add_library(${RUNTIME_MODULE_TARGET} ${_lib_type} ${RUNTIME_MODULE_SOURCES})
    add_library(${RUNTIME_DBG_MODULE_TARGET} ${_lib_type}
                                             ${RUNTIME_MODULE_SOURCES})
    target_link_libraries(${RUNTIME_MODULE_TARGET} PRIVATE lotto.h dice.h)
    target_link_libraries(${RUNTIME_DBG_MODULE_TARGET} PRIVATE lotto.h dice.h)

    if("${RUNTIME_MODULE_TYPE}" STREQUAL "PLUGIN")
        target_link_libraries(${RUNTIME_MODULE_TARGET} PRIVATE lotto)
        target_link_libraries(${RUNTIME_DBG_MODULE_TARGET} PRIVATE lotto-dbg)
    endif()

    # enable LTO for this module ?
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
                LOTTO_MODULE_NAME="${MODULE_NAME}" #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}" #
                LOGGER_DISABLE)
    target_compile_definitions(
        ${RUNTIME_DBG_MODULE_TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                LOTTO_RUNTIME_MODULE=1 #
                LOTTO_DRIVER_MODULE=0 #
                LOTTO_MODULE_NAME="${MODULE_NAME}" #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}")
    if(${RUNTIME_MODULE_TYPE} STREQUAL PLUGIN)
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
    if(NOT DEFINED MODULE_NAME)
        message(FATAL_ERROR "Undefined module name")
    endif()
    ensure_module_type(DRIVER_MODULE_TYPE_${MODULE_NAME})
    set(DRIVER_MODULE_TYPE ${DRIVER_MODULE_TYPE_${MODULE_NAME}})
    if("${DRIVER_MODULE_TYPE}" STREQUAL BUILTIN)
        set(_lib_type "OBJECT")
    else()
        set(_lib_type "SHARED")
    endif()

    set(DRIVER_MODULE_SOURCES ${ARGV})
    set(DRIVER_MODULE_TARGET lotto-driver-${MODULE_NAME})
    message(STATUS "Module ${MODULE_SLOT} (driver) : ${MODULE_NAME}")
    lotto_module_metadata_add(${MODULE_NAME} ${MODULE_SLOT} driver
                              ${DRIVER_MODULE_TYPE})

    add_library(${DRIVER_MODULE_TARGET} ${_lib_type} ${DRIVER_MODULE_SOURCES})
    target_link_libraries(${DRIVER_MODULE_TARGET} PRIVATE lotto.h dice.h)
    target_compile_definitions(
        ${DRIVER_MODULE_TARGET}
        PRIVATE DICE_MULTIFILE_MODULE #
                LOTTO_RUNTIME_MODULE=0 #
                LOTTO_DRIVER_MODULE=1 #
                LOTTO_MODULE_NAME="${MODULE_NAME}" #
                DICE_MODULE_SLOT=${MODULE_SLOT} #
                LOGGER_PREFIX="${MODULE_NAME}")
    if("${DRIVER_MODULE_TYPE}" STREQUAL "PLUGIN")
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
    ensure_module_name_prefix(${NAME})
    math(EXPR SLOT "${SLOT}+1")
    set(MODULE_NAME ${NAME})
    set(MODULE_SLOT ${SLOT})
endmacro()

macro(new_module_at NAME EXPLICIT_SLOT)
    ensure_module_name_prefix(${NAME})
    set(MODULE_NAME ${NAME})
    set(MODULE_SLOT ${EXPLICIT_SLOT})
endmacro()

add_custom_target(tikl-modules)
add_custom_target(clean-disabled-modules ALL)

function(add_tikl_module_target NAME)
    add_custom_target(tikl-module-${NAME})
    add_dependencies(tikl-test tikl-module-${NAME})
    add_dependencies(tikl-modules tikl-module-${NAME})
endfunction()

macro(add_module NAME)
    set(oneValueArgs SLOT DEFAULT)
    cmake_parse_arguments(ADD_MODULE "" "${oneValueArgs}" "" ${ARGN})
    if(DEFINED ADD_MODULE_DEFAULT)
        set(_add_module_default ${ADD_MODULE_DEFAULT})
    else()
        set(_add_module_default ON)
    endif()
    option(LOTTO_MODULE_${NAME} "Build Lotto with module ${NAME}"
           ${_add_module_default})

    if(${LOTTO_MODULE_${NAME}})
        set(MODULE_ROOT_NAME ${NAME})
        set(RUNTIME_MODULE_TYPE_${NAME} ${LOTTO_DEFAULT_RUNTIME_MODULE_TYPE})
        set(DRIVER_MODULE_TYPE_${NAME} ${LOTTO_DEFAULT_DRIVER_MODULE_TYPE})

        if(DEFINED ADD_MODULE_SLOT)
            new_module_at(${NAME} ${ADD_MODULE_SLOT})
        else()
            new_module(${NAME})
        endif()
        set(TIKL_MODULE_FEATURES "${TIKL_MODULE_FEATURES}\n-D module-${NAME}")
        add_tikl_module_target(${NAME})
        add_subdirectory(${NAME})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/include")
            install(
                DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/include/"
                DESTINATION "include"
                FILES_MATCHING
                PATTERN "*.h")
        endif()
        unset(MODULE_ROOT_NAME)
    else()
        set(_clean_target "clean-disabled-module-${NAME}")
        add_custom_target(
            ${_clean_target}
            COMMAND
                ${CMAKE_COMMAND}
                -DLOTTO_MODULE_BUILD_DIR=${LOTTO_MODULE_BUILD_DIR}
                -DMODULE_NAME=${NAME} -DSOEXT=${CMAKE_SHARED_LIBRARY_SUFFIX} -P
                ${PROJECT_SOURCE_DIR}/cmake/clean_disabled_module_artifacts.cmake
        )
        add_dependencies(clean-disabled-modules ${_clean_target})
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
    add_dependencies(tikl-${TEST} ${TARGET} deadline tikl-build lotto-cli)
    add_dependencies(tikl-module-${MODULE_NAME} tikl-${TEST})
endfunction()
