add_library(lotto-pos.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-pos.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-pos SHARED)
target_link_libraries(lotto-pos PRIVATE lotto-pos.o)
