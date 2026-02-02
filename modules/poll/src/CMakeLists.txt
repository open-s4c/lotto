add_library(lotto-poll.o OBJECT intercept.c handler.c module.c)
target_link_libraries(lotto-poll.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-poll SHARED)
target_link_libraries(lotto-poll PRIVATE lotto-poll.o)
