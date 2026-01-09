file(GLOB SRCS *.gdb)
foreach(SRC ${SRCS})
    get_filename_component(SRC_NAME ${SRC} NAME_WLE)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.gdb"
        COMMAND cp ${SRC} "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.gdb"
        COMMAND truncate -s +1 "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.gdb"
        DEPENDS ${SRC})
    make_blob(blob-${SRC_NAME} "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}.gdb" "")
endforeach()
