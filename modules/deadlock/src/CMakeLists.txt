add_library(lotto-deadlock.o OBJECT state.c handler.c module.c intercept.c)
target_link_libraries(lotto-deadlock.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-deadlock SHARED)
target_link_libraries(lotto-deadlock PRIVATE lotto-deadlock.o)
