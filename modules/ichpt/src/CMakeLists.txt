add_library(lotto-ichpt.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-ichpt.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-ichpt SHARED)
target_link_libraries(lotto-ichpt PRIVATE lotto-ichpt.o)

add_library(lotto-driver-ichpt SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-ichpt PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-ichpt)
