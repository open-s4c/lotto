add_library(lotto-blocking.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-blocking.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-blocking SHARED)
target_link_libraries(lotto-blocking PRIVATE lotto-blocking.o)
