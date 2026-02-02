add_library(lotto-atomic.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-atomic.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-atomic SHARED)
target_link_libraries(lotto-atomic PRIVATE lotto-atomic.o)
