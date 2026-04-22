file(GLOB SRCS stream*.c)
add_library(memock.o OBJECT memory_mock.c)
target_link_libraries(memock.o PRIVATE dl)
target_compile_definitions(memock.o PRIVATE -DLOTTO_TEST)

add_library(base_component SHARED)
target_link_libraries(base_component base_testing.o sys_testing.o
                      memmgr_runtime_libc.o memmgr_user_libc.o plan_testing.o)
target_compile_definitions(base_component PRIVATE LOGGER_PREFIX="base_cmpt")
