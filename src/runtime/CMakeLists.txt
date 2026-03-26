sanitize()
add_compile_definitions(_GNU_SOURCE)
set(MODULE_NAME runtime)
set(MODULE_SLOT 9)
add_compile_definitions(DICE_MULTIFILE_MODULE DICE_MODULE_SLOT=${MODULE_SLOT})

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

set(SRCS subscribe.c)
add_library(subscribers.o OBJECT ${SRCS})

set(SRCS memmgr_dice.c)
add_library(memmgr_runtime_dice.o OBJECT ${SRCS})
target_compile_definitions(memmgr_runtime_dice.o PRIVATE LOTTO_MEMMGR_RUNTIME)

# ##############################################################################
# libruntime
# ##############################################################################
set(SRCS ingress.c sighandler.c module.c malloc.c)
set(LIBS
    m
    mediator.o
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
    dice-tsan.o)

macro(add_runtime TARGET OUTPUT_NAME)
    add_library(${TARGET} SHARED ${SRCS})
    target_link_libraries(${TARGET} PRIVATE ${LIBS})
    set_target_properties(
        ${TARGET}
        PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_NAME
                   ${OUTPUT_NAME})
    install(TARGETS ${TARGET} DESTINATION lib)
    set(LIBTARGET "lib${OUTPUT_NAME}.so")
    make_blob(blob-${TARGET} ${LIBTARGET} ${TARGET})
endmacro()

add_runtime(lotto lotto-runtime)
add_runtime(lotto-verbose lotto-runtime-verbose)
target_compile_definitions(lotto PRIVATE -DLOGGER_DISABLE)

add_library(runtime-module.o OBJECT module.c)

function(add_lotto_builtin LIB VERBOSE_LIB)
    target_link_libraries(lotto PRIVATE ${LIB})
    target_link_libraries(lotto-verbose PRIVATE ${VERBOSE_LIB})
endfunction()
