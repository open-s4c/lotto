add_library(lotto-available.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-available.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-available SHARED)
target_link_libraries(lotto-available PRIVATE lotto-available.o)

add_library(lotto-driver-available SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-available PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-available)
