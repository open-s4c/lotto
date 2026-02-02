add_library(lotto-inactivity_timeout.o OBJECT state.c handler.c module.c)
target_link_libraries(lotto-inactivity_timeout.o PRIVATE lotto.h lotto-src.h
                                                         dice.h)

add_library(lotto-inactivity_timeout SHARED)
target_link_libraries(lotto-inactivity_timeout
                      PRIVATE lotto-inactivity_timeout.o)
