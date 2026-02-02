add_library(lotto-filtering.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-filtering.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-filtering SHARED)
target_link_libraries(lotto-filtering PRIVATE lotto-filtering.o)

add_library(lotto-driver-filtering SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-filtering PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-filtering)
