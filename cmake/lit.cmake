macro(add_lit_target TARGET WORKERS TIMEOUT FILES)
    list(LENGTH ${FILES} LEN)
    if("${LEN}")
        add_custom_target(${TARGET} COMMAND lit --verbose -j ${WORKERS}
                                            --timeout=${TIMEOUT} ${${FILES}})
    else()
        add_custom_target(${TARGET} COMMAND echo No tests)
    endif()
endmacro()

add_custom_target(lit-prepare)

macro(add_lit_prepare)
    get_filename_component(TARGET ${CMAKE_CURRENT_BINARY_DIR} NAME_WLE)
    add_custom_target(
        litdir-${TARGET}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/lit
        COMMAND ${CMAKE_COMMAND} -E copy ${LIT_CFG_PATH}
                ${CMAKE_CURRENT_BINARY_DIR}/lit)
    add_dependencies(lit-prepare litdir-${TARGET})
endmacro()
