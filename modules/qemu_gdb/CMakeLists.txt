include_directories(include)
add_subdirectory(src)
add_subdirectory(src/gdb/xml ${CMAKE_CURRENT_BINARY_DIR}/qemu-gdb-xml)
