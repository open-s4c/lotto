add_library(lotto-termination.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-termination.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-termination SHARED)
target_link_libraries(lotto-termination PRIVATE lotto-termination.o)
