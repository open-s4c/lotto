# Create a test case with racket.
function(add_racket_test)
    set(opts)
    set(ones FILENAME ARG)
    cmake_parse_arguments(_ "${opts}" "${ones}" "${mult}" ${ARGN})

    if(NOT DEFINED __FILENAME)
        message(FATAL_ERROR "No FILENAME in racket_test")
    endif()
    set(TARGET ${__FILENAME})

    if(NOT "${LOTTO_RACKET_TESTS}")
        return()
    endif()

    set(ARG_CMD "")
    if(DEFINED __ARG)
        set(ARG_CMD "++arg")
        set(TARGET ${__FILENAME}_${__ARG})
    endif()

    add_test(
        NAME ${TARGET}
        COMMAND env LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}/mock raco test
                ${ARG_CMD} ${__ARG} ${CMAKE_CURRENT_SOURCE_DIR}/${__FILENAME})
    if(DEFINED __BINDING)
        get_filename_component(NAME ${__BINDING} NAME_WLE)
        add_binding(TARGET ${NAME}.rkt INCLUDE ${__BINDING})
    endif()
endfunction()
