add_library(lotto-time.o OBJECT intercept.c module.c)
target_link_libraries(lotto-time.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-time SHARED)
target_link_libraries(lotto-time PRIVATE lotto-time.o)
