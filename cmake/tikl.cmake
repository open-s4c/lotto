set(LOTTO_TIKL_VERSION "0.3.1")
set(LOTTO_TIKL_URL
    "https://github.com/db7/tikl/archive/refs/tags/v${LOTTO_TIKL_VERSION}.tar.gz"
)
set(LOTTO_TIKL_SHA256
    "SHA256=af4485866062d3dd9666da442af4ba4a666f6a12748526a57f11e5daabc14e6d")

function(lotto_ensure_tikl OUT_TIKL OUT_TIKL_CHECK)
    if(DEFINED LOTTO_TIKL_EXECUTABLE
       AND DEFINED LOTTO_TIKL_CHECK_EXECUTABLE
       AND EXISTS "${LOTTO_TIKL_EXECUTABLE}"
       AND EXISTS "${LOTTO_TIKL_CHECK_EXECUTABLE}")
        # Cached values are valid; nothing further needed.
    else()
        unset(LOTTO_TIKL_EXECUTABLE CACHE)
        unset(LOTTO_TIKL_CHECK_EXECUTABLE CACHE)

        find_program(_tikl tikl)
        if(_tikl)
            find_program(_tikl_check tikl-check)
            if(NOT _tikl_check)
                message(
                    FATAL_ERROR
                        "Found tikl at ${_tikl} but tikl-check is missing. Install tikl-check or remove tikl from PATH to build the bundled version."
                )
            endif()
            set(LOTTO_TIKL_EXECUTABLE
                "${_tikl}"
                CACHE INTERNAL "tikl executable")
            set(LOTTO_TIKL_CHECK_EXECUTABLE
                "${_tikl_check}"
                CACHE INTERNAL "tikl-check executable")
            if(NOT TARGET tikl)
                add_custom_target(tikl)
            endif()
        else()
            set(_tikl_archive
                "${CMAKE_BINARY_DIR}/tikl-${LOTTO_TIKL_VERSION}.tar.gz")
            set(_tikl_source_dir
                "${CMAKE_BINARY_DIR}/tikl-${LOTTO_TIKL_VERSION}")
            set(_tikl_binary "${_tikl_source_dir}/tikl")
            set(_tikl_check "${_tikl_source_dir}/tikl-check")

            if(NOT EXISTS "${_tikl_archive}")
                file(
                    DOWNLOAD "${LOTTO_TIKL_URL}" "${_tikl_archive}"
                    SHOW_PROGRESS
                    EXPECTED_HASH "${LOTTO_TIKL_SHA256}")
            endif()

            if(NOT EXISTS "${_tikl_binary}")
                file(REMOVE_RECURSE "${_tikl_source_dir}")
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E tar xzf "${_tikl_archive}"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                    RESULT_VARIABLE _tikl_extract_rv)
                if(NOT _tikl_extract_rv EQUAL 0)
                    message(
                        FATAL_ERROR
                            "Failed to extract tikl ${LOTTO_TIKL_VERSION}")
                endif()

                execute_process(
                    COMMAND
                        ${CMAKE_C_COMPILER} -std=c11 -D_POSIX_C_SOURCE=200809L
                        -Wall -Wextra -Wpedantic -Wshadow -Werror -O2 -o tikl
                        tikl.c
                    WORKING_DIRECTORY "${_tikl_source_dir}"
                    RESULT_VARIABLE _tikl_build_rv)
                if(NOT _tikl_build_rv EQUAL 0)
                    message(
                        FATAL_ERROR
                            "Failed to compile tikl ${LOTTO_TIKL_VERSION}")
                endif()
            endif()

            if(NOT EXISTS "${_tikl_binary}")
                message(
                    FATAL_ERROR
                        "Expected tikl binary at ${_tikl_binary} was not created"
                )
            endif()

            if(NOT EXISTS "${_tikl_check}")
                message(
                    FATAL_ERROR
                        "Expected tikl-check script at ${_tikl_check} was not created"
                )
            endif()

            execute_process(
                COMMAND
                    python3 -c
                    "from pathlib import Path; path=Path(r\"${_tikl_check}\"); text=path.read_text(); text=text.replace(r\"s/[].[^$\\\\*/+?{}()|]/\\\\&/g\", r\"s/[].[^$\\\\*/+?{}()|]/\\\\\\\\&/g\"); path.write_text(text)"
                RESULT_VARIABLE _tikl_patch_rv)
            if(NOT _tikl_patch_rv EQUAL 0)
                message(
                    WARNING
                        "Failed to patch tikl-check escaping logic; proceeding anyway"
                )
            endif()

            set(LOTTO_TIKL_EXECUTABLE
                "${_tikl_binary}"
                CACHE INTERNAL "tikl executable")
            set(LOTTO_TIKL_CHECK_EXECUTABLE
                "${_tikl_check}"
                CACHE INTERNAL "tikl-check executable")
        endif()
    endif()

    if(NOT TARGET tikl)
        add_custom_target(tikl DEPENDS "${LOTTO_TIKL_EXECUTABLE}")
    endif()

    set(${OUT_TIKL}
        "${LOTTO_TIKL_EXECUTABLE}"
        PARENT_SCOPE)
    set(${OUT_TIKL_CHECK}
        "${LOTTO_TIKL_CHECK_EXECUTABLE}"
        PARENT_SCOPE)
endfunction()
