add_library(lotto-watchdog.o OBJECT module.c state.c handler.c)
target_link_libraries(lotto-watchdog.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-watchdog SHARED)
target_link_libraries(lotto-watchdog PRIVATE lotto-watchdog.o)
