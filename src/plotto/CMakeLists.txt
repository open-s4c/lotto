gen_pubsub_headers("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
include_directories(${CMAKE_BINARY_DIR}/generated)
add_compile_options(-fPIC -Wno-uninitialized)
add_compile_definitions(_GNU_SOURCE)

add_library(
    posix_interceptors.o OBJECT
    intercept_disable.c
    intercept_evec.c
    intercept_list.S
    intercept_log.c
    intercept_malloc.c
    intercept_mutex.c
    intercept_order.c
    intercept_posix.c
    intercept_pthread.c
    intercept_region_preemption.c
    intercept_region_rogue.c
    intercept_rsrc_deadlock.c
    intercept_task_velocity.c
    intercept_trace_pc.c
    intercept_tsan.c)

add_library(
    posix_interceptors_testing.o OBJECT
    intercept_disable.c
    intercept_evec.c
    intercept_list.S
    intercept_log.c
    intercept_malloc.c
    intercept_mutex.c
    intercept_order.c
    intercept_posix.c
    intercept_pthread.c
    intercept_region_preemption.c
    intercept_region_rogue.c
    intercept_rsrc_deadlock.c
    intercept_task_velocity.c
    intercept_trace_pc.c
    intercept_tsan.c)
target_compile_definitions(posix_interceptors_testing.o PUBLIC LOTTO_TEST)

# ##############################################################################
# libplotto
# ##############################################################################

macro(add_libplotto TARGET)
    add_library(${TARGET} SHARED plotto.c)
    set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                               ${CMAKE_BINARY_DIR})

    target_link_libraries(${TARGET} PRIVATE posix_interceptors.o pthread
                                            runtime)
    target_compile_options(${TARGET} PUBLIC -g)
    target_include_directories(${TARGET} PUBLIC ../include)

    if("${LOTTO_QEMU_YIELD}")
        target_link_libraries(${TARGET} PRIVATE qlotto.o)
    endif()

    install(TARGETS ${TARGET} DESTINATION lib)
    set(LIBTARGET lib${TARGET}.so)
    make_blob(blob-${TARGET} ${LIBTARGET} ${TARGET})

endmacro()

add_libplotto(plotto)
add_libplotto(plotto-verbose)
target_compile_definitions(plotto-verbose PRIVATE -DLOTTO_VERBOSE)

# ##############################################################################
# libtrace_pc
# ##############################################################################
add_library(trace_pc SHARED trace_pc.c)
install(TARGETS trace_pc DESTINATION lib)
