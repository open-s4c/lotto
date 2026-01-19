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

# ##############################################################################
# libruntime
# ##############################################################################
set(SRCS runtime.c sighandler.c)
set(LIBTARGET "libruntime.so")
add_library(runtime SHARED ${SRCS})
target_link_libraries(runtime PRIVATE mediator.o interceptor.o switcher.o
                                      engine m)
set_target_properties(runtime PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                         ${CMAKE_BINARY_DIR})

target_sources(
    runtime
    PRIVATE $<TARGET_OBJECTS:dice.o>
            # $<TARGET_OBJECTS:dice-self.o>
            # $<TARGET_OBJECTS:dice-pthread_create.o>
            # $<TARGET_OBJECTS:dice-pthread_mutex.o>
            # $<TARGET_OBJECTS:dice-pthread_cond.o>
            # $<TARGET_OBJECTS:dice-tsan.o>
)

install(TARGETS runtime DESTINATION lib)
make_blob(blob-runtime ${LIBTARGET} runtime)
