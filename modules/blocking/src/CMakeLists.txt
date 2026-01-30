add_library(lotto-blocking.o OBJECT state.c handler.c)
target_link_libraries(lotto-blocking.o PRIVATE lotto.h lotto-src.h)

add_library(lotto-blocking SHARED state.c handler.c)
target_link_libraries(lotto-blocking PRIVATE lotto.h lotto-src.h)
