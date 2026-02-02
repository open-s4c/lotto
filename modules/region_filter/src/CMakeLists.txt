add_library(lotto-region_filter.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-region_filter.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-region_filter SHARED)
target_link_libraries(lotto-region_filter PRIVATE lotto-region_filter.o)
