add_library(lotto-region_filter.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-region_filter.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-region_filter SHARED)
target_link_libraries(lotto-region_filter PRIVATE lotto-region_filter.o)

add_library(lotto-driver-region_filter SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-region_filter PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-region_filter)
