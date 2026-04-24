file(GLOB SRCS *.xml)
foreach(SRC ${SRCS})
    get_filename_component(SRC_NAME ${SRC} NAME_WLE)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.xml"
        COMMAND cp ${SRC} "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.xml"
        COMMAND truncate -s +1 "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.xml"
        DEPENDS ${SRC})
    make_blob(blob-${SRC_NAME} "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.xml" "")
endforeach()
