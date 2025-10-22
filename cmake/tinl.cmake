set(LOTTO_TINL_VERSION "0.1.1")
set(LOTTO_TINL_URL
    "https://github.com/db7/tinl/archive/refs/tags/v${LOTTO_TINL_VERSION}.tar.gz"
)
set(LOTTO_TINL_SHA256
    "SHA256=151b2b0972006c8e49fd8bf8624aabb3e78009c6745f4cb4edfe5f1a40c6dc8c")

function(lotto_ensure_tinl OUT_TINL OUT_TINL_CHECK)
    if(NOT DEFINED LOTTO_TINL_EXECUTABLE OR NOT DEFINED LOTTO_TINL_CHECK_EXECUTABLE)
        find_program(_tinl tinl)
        if(_tinl)
            find_program(_tinl_check tinl-check)
            if(NOT _tinl_check)
                message(
                    FATAL_ERROR
                        "Found tinl at ${_tinl} but tinl-check is missing. Install tinl-check or remove tinl from PATH to build the bundled version."
                )
            endif()
            set(LOTTO_TINL_EXECUTABLE "${_tinl}" CACHE INTERNAL "tinl executable")
            set(LOTTO_TINL_CHECK_EXECUTABLE "${_tinl_check}" CACHE INTERNAL
                                                             "tinl-check executable")
            if(NOT TARGET tinl)
                add_custom_target(tinl)
            endif()
        else()
            set(_tinl_archive "${CMAKE_BINARY_DIR}/tinl-${LOTTO_TINL_VERSION}.tar.gz")
            set(_tinl_source_dir "${CMAKE_BINARY_DIR}/tinl-${LOTTO_TINL_VERSION}")

            if(NOT EXISTS "${_tinl_archive}")
                file(
                    DOWNLOAD "${LOTTO_TINL_URL}" "${_tinl_archive}"
                    SHOW_PROGRESS
                    EXPECTED_HASH "${LOTTO_TINL_SHA256}")
            endif()

            set(_tinl_source_stamp "${_tinl_source_dir}/tinl.c")
            if(NOT TARGET tinl_extract)
                add_custom_command(
                    OUTPUT "${_tinl_source_stamp}"
                    COMMAND ${CMAKE_COMMAND} -E remove_directory "${_tinl_source_dir}"
                    COMMAND ${CMAKE_COMMAND} -E tar xzf "${_tinl_archive}"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                    DEPENDS "${_tinl_archive}"
                    COMMENT "Extracting tinl ${LOTTO_TINL_VERSION}"
                    VERBATIM)
                add_custom_target(tinl_extract DEPENDS "${_tinl_source_stamp}")
            endif()

            if(NOT TARGET tinl)
                add_custom_command(
                    OUTPUT "${_tinl_source_dir}/tinl"
                    COMMAND
                        ${CMAKE_C_COMPILER} -std=c11 -D_POSIX_C_SOURCE=200809L -Wall
                        -Wextra -Wpedantic -Wshadow -Werror -O2 -o tinl tinl.c
                    WORKING_DIRECTORY "${_tinl_source_dir}"
                    DEPENDS tinl_extract
                    COMMENT "Compiling tinl ${LOTTO_TINL_VERSION}"
                    VERBATIM)
                add_custom_target(tinl DEPENDS "${_tinl_source_dir}/tinl")
            endif()

            set(LOTTO_TINL_EXECUTABLE "${_tinl_source_dir}/tinl"
                CACHE INTERNAL "tinl executable")
            set(LOTTO_TINL_CHECK_EXECUTABLE "${_tinl_source_dir}/tinl-check"
                CACHE INTERNAL "tinl-check executable")
        endif()
    elseif(NOT TARGET tinl)
        add_custom_target(tinl)
    endif()

    set(${OUT_TINL} "${LOTTO_TINL_EXECUTABLE}" PARENT_SCOPE)
    set(${OUT_TINL_CHECK} "${LOTTO_TINL_CHECK_EXECUTABLE}" PARENT_SCOPE)
endfunction()
