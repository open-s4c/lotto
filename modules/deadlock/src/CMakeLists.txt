add_library(lotto-deadlock.o OBJECT state.c handler.c module.c intercept.c)
target_link_libraries(lotto-deadlock.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-deadlock SHARED)
target_link_libraries(lotto-deadlock PRIVATE lotto-deadlock.o)

add_library(lotto-driver-deadlock SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-deadlock PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-deadlock)

