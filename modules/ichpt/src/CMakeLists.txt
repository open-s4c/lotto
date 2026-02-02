add_library(lotto-ichpt.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-ichpt.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-ichpt SHARED)
target_link_libraries(lotto-ichpt PRIVATE lotto-ichpt.o)
