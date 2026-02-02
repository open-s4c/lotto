add_library(lotto-order.o OBJECT intercept.c module.c)
target_link_libraries(lotto-order.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-order SHARED)
target_link_libraries(lotto-order PRIVATE lotto-order.o)
