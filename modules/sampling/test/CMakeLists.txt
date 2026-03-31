file(GLOB SRCS *.c)
list(FILTER SRCS EXCLUDE REGEX "parser_test\\.c$")

foreach(SRC ${SRCS})
    add_module_tikl_test(${SRC})
endforeach()

add_executable(sampling_parser_test parser_test.c ../src/parser.c)
target_include_directories(
    sampling_parser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                 ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(
    sampling_parser_test PRIVATE base.o sys.o dice.o memmgr_runtime_libc.o
                                 memmgr_user_libc.o)
add_test(NAME sampling_parser_test COMMAND sampling_parser_test)
