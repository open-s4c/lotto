add_library(lotto-race.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-race.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-race SHARED)
target_link_libraries(lotto-race PRIVATE lotto-race.o)
