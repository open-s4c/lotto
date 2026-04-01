add_mock(
    TARGET memory_mock.c
    INCLUDE lotto/sys/memory.h lotto/sys/memmgr_runtime.h
    EXCLUDE io.h logger.h FLAGS -I ${PROJECT_SOURCE_DIR}/deps/target/include)
file(GLOB SRCS stream*.c)
add_library(memock.o OBJECT memory_mock.c)
target_link_libraries(memock.o PRIVATE dl)
target_compile_definitions(memock.o PRIVATE -DLOTTO_TEST)

add_library(base_component SHARED)
target_link_libraries(base_component base_testing.o sys_testing.o
                      memmgr_runtime_libc.o memmgr_user_libc.o plan_testing.o)
target_compile_definitions(base_component PRIVATE LOGGER_PREFIX="base_cmpt")

add_binding(
    TARGET tidmap.rkt
    INCLUDE lotto/base/tidmap.h
    INCLUDE lotto/sys/logger.h
    EXCLUDE sys/types.h)

add_binding(
    TARGET bag.rkt
    INCLUDE lotto/base/bag.h
    INCLUDE lotto/sys/logger.h
    EXCLUDE sys/types.h)

add_binding(
    TARGET stable_address.rkt
    INCLUDE lotto/base/stable_address.h
    EXCLUDE sys/types.h)

add_binding(
    TARGET trace_flat.rkt
    INCLUDE lotto/base/trace_flat.h
    INCLUDE lotto/base/trace.h
    EXCLUDE sys/types.h)

add_binding(
    TARGET plan.rkt
    INCLUDE lotto/engine/plan.h
    EXCLUDE sys/types.h)

add_binding(
    TARGET tidbag.rkt
    INCLUDE lotto/base/tidbag.h
    EXCLUDE sys/types.h)

add_binding(
    TARGET tidset.rkt
    INCLUDE lotto/base/tidset.h
    EXCLUDE sys/types.h)
