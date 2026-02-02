add_library(lotto-filtering.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-filtering.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-filtering SHARED)
target_link_libraries(lotto-filtering PRIVATE lotto-filtering.o)
