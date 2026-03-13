sanitize()
add_compile_definitions(_GNU_SOURCE)
set(SRCS mediator.c)
add_library(mediator.o OBJECT ${SRCS})
add_library(mediator_testing.o OBJECT ${SRCS})
target_compile_definitions(mediator_testing.o PRIVATE -DLOTTO_TEST)
target_compile_options(mediator.o PRIVATE -Wno-uninitialized)
target_compile_options(mediator_testing.o PRIVATE -Wno-uninitialized)

set(SRCS switcher.c)
add_library(switcher.o OBJECT ${SRCS})
add_library(switcher_testing.o OBJECT ${SRCS})
target_compile_definitions(switcher_testing.o PRIVATE -DLOTTO_TEST)

set(SRCS interceptor.c thread_id.c)
add_library(interceptor.o OBJECT ${SRCS})
add_library(interceptor_testing.o OBJECT ${SRCS})
target_compile_definitions(interceptor_testing.o PRIVATE -DLOTTO_TEST)

set(SRCS subscribe.c)
add_library(subscribers.o OBJECT ${SRCS})

set(SRCS memmgr_dice.c)
add_library(memmgr_runtime_dice.o OBJECT ${SRCS})
target_compile_definitions(memmgr_runtime_dice.o PRIVATE LOTTO_MEMMGR_RUNTIME)

# ##############################################################################
# libruntime
# ##############################################################################
set(SRCS runtime.c sighandler.c)
set(LIBS
    m
    mediator.o
    interceptor.o
    switcher.o
    sys.o
    base.o
    memmgr_runtime_dice.o
    memmgr_user_libc.o
    subscribers.o
    lotto-runtime-engine.o
    dice.o
    dice-cxa.o
    dice-mman.o
    dice-pthread_cond.o
    dice-pthread_create.o
    dice-pthread_mutex.o
    dice-pthread_rwlock.o
    dice-pthread_spinlock.o
    dice-self.o
    dice-sem.o
    dice-stacktrace.o
    dice-malloc.o
    dice-tsan.o)

macro(add_runtime TARGET)
    add_library(${TARGET} SHARED ${SRCS})
    target_link_libraries(${TARGET} PRIVATE ${LIBS})
    target_compile_definitions(${TARGET} PRIVATE DICE_MODULE_SLOT=9)
    set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                               ${CMAKE_BINARY_DIR})
    install(TARGETS ${TARGET} DESTINATION lib)
    set(LIBTARGET "lib${TARGET}.so")
    make_blob(blob-${TARGET} ${LIBTARGET} ${TARGET})
endmacro()

add_runtime(lotto)
add_runtime(lotto-verbose)
target_compile_definitions(lotto-verbose PRIVATE -DLOTTO_VERBOSE)

function(add_lotto_builtin LIB)
    target_link_libraries(lotto PRIVATE ${LIB})
    target_link_libraries(lotto-verbose PRIVATE ${LIB})
endfunction()
