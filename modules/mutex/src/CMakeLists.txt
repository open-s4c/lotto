add_library(lotto-mutex.o OBJECT module.c state.c handler.c intercept_mutex.c
                                 intercept_pthread_mutex.c)
target_link_libraries(lotto-mutex.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-mutex SHARED)
target_link_libraries(lotto-mutex PRIVATE lotto-mutex.o)
