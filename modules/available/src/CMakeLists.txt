add_library(lotto-available.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-available.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-available SHARED)
target_link_libraries(lotto-available PRIVATE lotto-available.o)

add_library(lotto-available-cli.o OBJECT state.c flags.c module.c)
target_link_libraries(lotto-available-cli.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-available-cli SHARED)
target_link_libraries(lotto-available-cli PRIVATE lotto-available-cli.o)

target_link_libraries(lotto-cli PRIVATE lotto-available-cli)
