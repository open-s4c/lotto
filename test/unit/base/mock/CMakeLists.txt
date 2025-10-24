add_mock(
    TARGET memory_mock.c
    INCLUDE lotto/sys/memory.h lotto/sys/memmgr_runtime.h
    EXCLUDE io.h logger.h FLAGS -I ${PROJECT_SOURCE_DIR}/deps/target/include)
file(GLOB SRCS stream*.c)
add_library(memock.o OBJECT memory_mock.c)
target_link_libraries(memock.o PRIVATE dl)
target_compile_definitions(memock.o PRIVATE -DLOTTO_TEST)
