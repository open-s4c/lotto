cmake_minimum_required(VERSION 3.16)

project(
    lotto
    VERSION 3.0.0
    LANGUAGES C)

set(DICE_LOG_LEVEL
    "INFO"
    CACHE STRING "" FORCE)

set(LOTTO_LOG_LEVEL
    "DEBUG"
    CACHE STRING "" FORCE)

set(LOTTO_DICE_UPSTREAM_DIR "${CMAKE_SOURCE_DIR}/deps/dice")
set(LOTTO_DICE_PATCH_FILE
    "${CMAKE_SOURCE_DIR}/patches/dice/0001-Adjust-dice-for-lotto-integration.patch"
)
set(LOTTO_DICE_SOURCE_DIR "${CMAKE_BINARY_DIR}/deps/dice")
set(LOTTO_DICE_BINARY_DIR "${CMAKE_BINARY_DIR}/deps/dice-build")

if(FALSE)
    file(REMOVE_RECURSE "${LOTTO_DICE_SOURCE_DIR}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${LOTTO_DICE_UPSTREAM_DIR}" "${LOTTO_DICE_SOURCE_DIR}"
        RESULT_VARIABLE _dice_copy_result)
    if(NOT _dice_copy_result EQUAL 0)
        message(FATAL_ERROR "Failed to stage dice sources for patching.")
    endif()
endif()

if(APPLE)
    find_program(PATCH_EXECUTABLE patch)
    if(NOT PATCH_EXECUTABLE)
        message(
            FATAL_ERROR
                "Required tool 'patch' not found. Please install it to build lotto."
        )
    endif()

    execute_process(
        COMMAND "${PATCH_EXECUTABLE}" -p1 -i "${LOTTO_DICE_PATCH_FILE}"
        WORKING_DIRECTORY "${LOTTO_DICE_SOURCE_DIR}"
        RESULT_VARIABLE _dice_patch_result
        OUTPUT_VARIABLE _dice_patch_stdout
        ERROR_VARIABLE _dice_patch_stderr)
    if(NOT _dice_patch_result EQUAL 0)
        message(
            FATAL_ERROR
                "Failed to apply lotto dice patch.\nstdout: ${_dice_patch_stdout}\nstderr: ${_dice_patch_stderr}"
        )
    endif()
endif()

set(LOTTO_DICE_INCLUDE_DIR "${LOTTO_DICE_SOURCE_DIR}/include")
set(LOTTO_DICE_SCRIPTS_DIR "${LOTTO_DICE_SOURCE_DIR}/scripts")

add_subdirectory(deps/dice dice)

include_directories(deps/dice/include)
include_directories(deps/dice/deps/libvsync/include)
include_directories(include)
include_directories(src/include)
add_compile_definitions(LOG_LEVEL=${LOTTO_LOG_LEVEL})
add_subdirectory(src)

enable_testing()
add_subdirectory(test)
