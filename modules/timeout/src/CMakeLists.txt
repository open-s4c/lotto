add_library(lotto-timeout.o OBJECT module.c handler.c)
target_link_libraries(lotto-timeout.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-timeout SHARED)
target_link_libraries(lotto-timeout PRIVATE lotto-timeout.o)
