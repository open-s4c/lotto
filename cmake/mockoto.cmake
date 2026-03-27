add_custom_target(mockoto)

function(_mockoto_resolve_headers OUT_VAR)
    set(_resolved)
    foreach(_hdr ${ARGN})
        if(IS_ABSOLUTE "${_hdr}")
            list(APPEND _resolved "${_hdr}")
        elseif(EXISTS "${PROJECT_SOURCE_DIR}/src/include/${_hdr}")
            list(APPEND _resolved "${PROJECT_SOURCE_DIR}/src/include/${_hdr}")
        elseif(EXISTS "${PROJECT_SOURCE_DIR}/include/${_hdr}")
            list(APPEND _resolved "${PROJECT_SOURCE_DIR}/include/${_hdr}")
        elseif(EXISTS "${PROJECT_BINARY_DIR}/generated/${_hdr}")
            list(APPEND _resolved "${PROJECT_BINARY_DIR}/generated/${_hdr}")
        else()
            list(APPEND _resolved "${_hdr}")
        endif()
    endforeach()
    set(${OUT_VAR} ${_resolved} PARENT_SCOPE)
endfunction()

function(add_mockoto)
    set(opts RACKET)
    set(ones TARGET)
    set(mult INCLUDE EXCLUDE FLAGS DEPENDENCIES)
    cmake_parse_arguments(_ "${opts}" "${ones}" "${mult}" ${ARGN})

    if(NOT DEFINED __TARGET)
        message(FATAL_ERROR "No TARGET in add_mock")
    endif()

    if(DEFINED __EXCLUDE)
        list(TRANSFORM __EXCLUDE PREPEND "--path-exclude;")
    endif()

    _mockoto_resolve_headers(__INCLUDE_RESOLVED ${__INCLUDE})
    set(__MOCKOTO_INCLUDE_FLAGS)
    foreach(_inc ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
        list(APPEND __MOCKOTO_INCLUDE_FLAGS -I ${_inc})
    endforeach()

    if("${__RACKET}")
        add_custom_target(
            update-${__TARGET} ALL
            COMMAND
                mockoto -p ${PROJECT_BINARY_DIR} #
                ${__EXCLUDE} --mode rkt ${__INCLUDE_RESOLVED} -- #
                ${__MOCKOTO_INCLUDE_FLAGS} #
                -I ${PROJECT_SOURCE_DIR}/include #
                -I ${PROJECT_SOURCE_DIR}/src/include #
                -I ${PROJECT_SOURCE_DIR}/deps/dice/include #
                -I ${PROJECT_SOURCE_DIR}/deps/dice/deps/libvsync/include #
                ${__FLAGS} #
                > ${CMAKE_CURRENT_SOURCE_DIR}/${__TARGET}
            COMMAND
                /bin/sh ${PROJECT_SOURCE_DIR}/scripts/mockoto-rkt-sanitize.sh
                ${CMAKE_CURRENT_SOURCE_DIR}/${__TARGET}
            BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/${__TARGET})
    else()
        string(REGEX REPLACE "[.]c$" ".h" HEADER ${__TARGET})
        add_custom_target(
            update-${__TARGET} ALL
            COMMAND
                mockoto -p ${PROJECT_BINARY_DIR} #
                ${__EXCLUDE} --mode C ${__INCLUDE_RESOLVED} -- #
                ${__MOCKOTO_INCLUDE_FLAGS} #
                -I ${PROJECT_SOURCE_DIR}/include #
                -I ${PROJECT_SOURCE_DIR}/src/include #
                -I ${PROJECT_SOURCE_DIR}/deps/dice/include #
                -I ${PROJECT_SOURCE_DIR}/deps/dice/deps/libvsync/include #
                ${__FLAGS} #
                > ${CMAKE_CURRENT_SOURCE_DIR}/${__TARGET}
            COMMAND
                mockoto -p ${PROJECT_BINARY_DIR} #
                ${__EXCLUDE} --mode H ${__INCLUDE_RESOLVED} -- #
                ${__MOCKOTO_INCLUDE_FLAGS} #
                -I ${PROJECT_SOURCE_DIR}/include #
                -I ${PROJECT_SOURCE_DIR}/src/include #
                -I ${PROJECT_SOURCE_DIR}/deps/dice/include #
                -I ${PROJECT_SOURCE_DIR}/deps/dice/deps/libvsync/include #
                ${__FLAGS} #
                > ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER}
            BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER}
                       ${CMAKE_CURRENT_SOURCE_DIR}/${__TARGET})
    endif()

    add_dependencies(mockoto update-${__TARGET})
endfunction()

function(add_mock)
    add_mockoto(${ARGN})
endfunction()

function(add_binding)
    add_mockoto(RACKET ${ARGN})
endfunction()
