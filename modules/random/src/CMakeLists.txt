add_library(lotto-random.o OBJECT intercept.c module.c)
target_link_libraries(lotto-random.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-random SHARED)
target_link_libraries(lotto-random PRIVATE lotto-random.o)
