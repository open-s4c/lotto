add_library(lotto-mutex.o OBJECT module.c state.c handler.c intercept_mutex.c
                                 intercept_pthread_mutex.c)
target_link_libraries(lotto-mutex.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-mutex SHARED)
target_link_libraries(lotto-mutex PRIVATE lotto-mutex.o)

add_library(lotto-driver-mutex SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-mutex PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-mutex)
