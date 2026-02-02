add_library(lotto-yield.o OBJECT module.c state.c handler.c intercept_yield.c
                                 intercept.c)
target_link_libraries(lotto-yield.o PRIVATE lotto.h lotto-src.h)

add_library(lotto-yield SHARED)
target_link_libraries(lotto-yield PRIVATE lotto-yield.o)
