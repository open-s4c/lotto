# prepare base component add_library(base_component SHARED base_mock.c)
add_library(base_component SHARED)
target_link_libraries(base_component base_testing.o sys_testing.o
                      memmgr_runtime.o memmgr_user.o)

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
    INCLUDE lotto/base/plan.h
    EXCLUDE sys/types.h)

add_binding(
    TARGET tidbag.rkt
    INCLUDE lotto/base/tidbag.h
    EXCLUDE sys/types.h)

add_binding(
    TARGET tidset.rkt
    INCLUDE lotto/base/tidset.h
    EXCLUDE sys/types.h)
