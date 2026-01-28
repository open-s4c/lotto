add_library(lotto-src.h INTERFACE)
target_include_directories(lotto-src.h INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-file-name=libasan.so
    OUTPUT_VARIABLE LIBASAN_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
file(GLOB_RECURSE SRCS *.in)
foreach(SRC ${SRCS})
    get_filename_component(SRC_NAME ${SRC} NAME_WLE)
    get_filename_component(SRC_DIR ${SRC} DIRECTORY)
    file(RELATIVE_PATH RELATIVE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${SRC_DIR})
    configure_file(
        ${SRC} ${CMAKE_BINARY_DIR}/generated/${RELATIVE_DIR}/${SRC_NAME} @ONLY)
endforeach()
