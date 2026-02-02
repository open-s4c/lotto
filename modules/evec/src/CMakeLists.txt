add_library(lotto-evec.o OBJECT module.c handler.c intercept_evec.c
                                intercept_pthread_cond.c)
target_link_libraries(lotto-evec.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-evec SHARED)
target_link_libraries(lotto-evec PRIVATE lotto-evec.o)
