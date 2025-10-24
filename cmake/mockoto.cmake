add_custom_target(mockoto)

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

    if("${__RACKET}")
        add_custom_target(
            update-${__TARGET} ALL
            COMMAND
                mockoto -p ${PROJECT_BINARY_DIR} #
                ${__EXCLUDE} --mode rkt ${__INCLUDE} -- #
                -I ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES} #
                -I ${PROJECT_SOURCE_DIR}/include #
                -I ${PROJECT_SOURCE_DIR}/src/include #
                -I ${PROJECT_SOURCE_DIR}/deps/libvsync/include #
                -I ${PROJECT_SOURCE_DIR}/deps/libvsync/vatomic/include #
                ${__FLAGS} #
                > ${CMAKE_CURRENT_SOURCE_DIR}/${__TARGET}
            BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/${__TARGET})
    else()
        string(REGEX REPLACE "[.]c$" ".h" HEADER ${__TARGET})
        add_custom_target(
            update-${__TARGET} ALL
            COMMAND
                mockoto -p ${PROJECT_BINARY_DIR} #
                ${__EXCLUDE} --mode C ${__INCLUDE} -- #
                -I ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES} #
                -I ${PROJECT_SOURCE_DIR}/include #
                -I ${PROJECT_SOURCE_DIR}/src/include #
                -I ${PROJECT_SOURCE_DIR}/deps/libvsync/include #
                -I ${PROJECT_SOURCE_DIR}/deps/libvsync/vatomic/include #
                ${__FLAGS} #
                > ${CMAKE_CURRENT_SOURCE_DIR}/${__TARGET}
            COMMAND
                mockoto -p ${PROJECT_BINARY_DIR} #
                ${__EXCLUDE} --mode H ${__INCLUDE} -- #
                -I ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES} #
                -I ${PROJECT_SOURCE_DIR}/include #
                -I ${PROJECT_SOURCE_DIR}/src/include #
                -I ${PROJECT_SOURCE_DIR}/deps/libvsync/include #
                -I ${PROJECT_SOURCE_DIR}/deps/libvsync/vatomic/include #
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
