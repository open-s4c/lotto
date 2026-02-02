add_library(lotto-region_preemption.o OBJECT state.c handler.c module.c
                                             intercept.c)
target_link_libraries(lotto-region_preemption.o PRIVATE lotto.h lotto-src.h
                                                        dice.h)

add_library(lotto-region_preemption SHARED)
target_link_libraries(lotto-region_preemption PRIVATE lotto-region_preemption.o)
