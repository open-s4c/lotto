add_library(lotto-disable.o OBJECT intercept.c module.c)
target_link_libraries(lotto-disable.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-disable SHARED)
target_link_libraries(lotto-disable PRIVATE lotto-disable.o)
