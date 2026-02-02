add_library(lotto-region_preemption.o OBJECT state.c handler.c module.c
                                             intercept.c)
target_link_libraries(lotto-region_preemption.o PRIVATE lotto.h lotto-src.h
                                                        dice.h)

add_library(lotto-region_preemption SHARED)
target_link_libraries(lotto-region_preemption PRIVATE lotto-region_preemption.o)

add_library(lotto-driver-region_preemption SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-region_preemption PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-region_preemption)
