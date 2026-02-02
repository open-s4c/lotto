add_library(lotto-enforce.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-enforce.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-enforce SHARED)
target_link_libraries(lotto-enforce PRIVATE lotto-enforce.o)
