add_library(lotto-pct.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-pct.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-pct SHARED)
target_link_libraries(lotto-pct PRIVATE lotto-pct.o)
