add_custom_target(
    pollen
    COMMAND ${CMAKE_COMMAND} -S${PROJECT_SOURCE_DIR}
            -B${CMAKE_CURRENT_BINARY_DIR}/build
    COMMAND
        ${CMAKE_COMMAND} -S${PROJECT_SOURCE_DIR}
        -B${CMAKE_CURRENT_BINARY_DIR}/build -LAH >
        ${CMAKE_CURRENT_BINARY_DIR}/output.log
    COMMAND touch ${CMAKE_CURRENT_SOURCE_DIR}/build-options.md.pp
    COMMAND raco pollen render ${CMAKE_CURRENT_SOURCE_DIR}/build-options.md.pp
            ${CMAKE_CURRENT_BINARY_DIR}/output.log
    COMMENT "Generating additonal Markdown documentation")
