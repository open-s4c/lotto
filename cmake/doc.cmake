# ##############################################################################
# Doxygen documentation
# ##############################################################################

function(add_doc_target target src_dir distro_name doc_out_dir)
return()
    if(NOT src_dir)
        message(FATAL_ERROR "required parameter src_dir is empty")
    endif()
    if(NOT doc_out_dir)
        message(FATAL_ERROR "required parameter doc_out_dir is empty")
    endif()
    message(DEBUG "Adding documentation target for ${src_dir}")
    pkgtools_get_docker_command(DOXYGEN_DOCKER "s4c/utils/docker/doxygen")
    set(DOXYGEN_INPUT "${src_dir}")
    if("${distro_name}" STREQUAL "main")
        set(command_prefix "")
    else()
        set(command_prefix "${distro_name}-distro-")
    endif()
    set(DOXYGEN_OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/${command_prefix}doxygen-${target}")
    file(MAKE_DIRECTORY "${DOXYGEN_OUTPUT}")
    set(DOXYFILE_IN "${PROJECT_SOURCE_DIR}/doc/Doxyfile.in")
    set(DOXYFILE
        "${CMAKE_CURRENT_BINARY_DIR}/${command_prefix}Doxyfile-${target}")
    configure_file(${DOXYFILE_IN} ${DOXYFILE})

    add_custom_target(
        "${command_prefix}doxygen-${target}"
        COMMAND ${DOXYGEN_DOCKER} doxygen ${DOXYFILE}
        WORKING_DIRECTORY "${DOXYGEN_INPUT}"
        COMMENT "Generating Doxygen documentation"
        VERBATIM)

    install(
        DIRECTORY ${DOXYGEN_OUTPUT}/man
        DESTINATION share/man
        COMPONENT "ManPages"
        EXCLUDE_FROM_ALL)

    # ##########################################################################
    # Markdown generation
    # ##########################################################################

    add_custom_target(
        "${command_prefix}markdown-${target}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${doc_out_dir}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${doc_out_dir}"
        COMMAND ${DOXYGEN_DOCKER} mdox -i ${DOXYGEN_OUTPUT}/xml -o
                "${doc_out_dir}"
        COMMENT "Generating Markdown documentation for ${DOXYGEN_INPUT}"
        VERBATIM
        DEPENDS "${command_prefix}doxygen-${target}" lotto_unsafe_sys)

endfunction()
