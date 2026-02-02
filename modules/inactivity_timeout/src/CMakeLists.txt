add_library(lotto-inactivity_timeout.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-inactivity_timeout.o PRIVATE lotto.h lotto-src.h
                                                         dice.h)

add_library(lotto-inactivity_timeout SHARED)
target_link_libraries(lotto-inactivity_timeout
                      PRIVATE lotto-inactivity_timeout.o)

add_library(lotto-driver-inactivity_timeout SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-inactivity_timeout PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-inactivity_timeout)

