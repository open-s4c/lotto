add_library(lotto-tsan.o OBJECT intercept.c module.c)
target_link_libraries(lotto-tsan.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-tsan SHARED)
target_link_libraries(lotto-tsan PRIVATE lotto-tsan.o)
