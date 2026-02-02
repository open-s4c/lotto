add_library(lotto-impasse.o OBJECT handler.c module.c)
target_link_libraries(lotto-impasse.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-impasse SHARED)
target_link_libraries(lotto-impasse PRIVATE lotto-impasse.o)
