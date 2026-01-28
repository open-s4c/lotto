gen_pubsub_headers("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
include_directories(${CMAKE_BINARY_DIR}/generated)
add_compile_options(-fPIC -Wno-uninitialized)
add_compile_definitions(_GNU_SOURCE)

if(OFF)
    add_library(
        posix_interceptors.o OBJECT
        intercept_disable.c
        intercept_evec.c
        intercept_list.S
        intercept_malloc.c
        intercept_mutex.c
        intercept_order.c
        intercept_posix.c
        intercept_pthread.c
        intercept_region_preemption.c
        intercept_region_rogue.c
        intercept_rsrc_deadlock.c
        intercept_task_velocity.c
        intercept_tsan.c)

    add_library(
        posix_interceptors_testing.o OBJECT
        intercept_disable.c
        intercept_evec.c
        intercept_list.S
        intercept_malloc.c
        intercept_mutex.c
        intercept_order.c
        intercept_posix.c
        intercept_pthread.c
        intercept_region_preemption.c
        intercept_region_rogue.c
        intercept_rsrc_deadlock.c
        intercept_task_velocity.c
        intercept_tsan.c)
    target_compile_definitions(posix_interceptors_testing.o PUBLIC LOTTO_TEST)
endif()

file(GLOB SRCS intercept_*)
set(SRCS
    intercept_disable.c
    intercept_evec.c
    intercept_list.S
    intercept_malloc.c
    intercept_mutex.c
    intercept_order.c
    intercept_posix.c
    # intercept_pthread.c
    intercept_pthread_cond.c
    intercept_pthread_create.c
    intercept_pthread_mutex.c
    # intercept_pthread_setspecific.c
    intercept_region_preemption.c
    intercept_region_rogue.c
    intercept_rsrc_deadlock.c
    intercept_sched_yield.c
    intercept_task_velocity.c
    intercept_tsan.c
    intercept_yield.c)
add_library(interceptors.o OBJECT ${SRCS})
