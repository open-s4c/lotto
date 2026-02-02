add_library(lotto-task_velocity.o OBJECT state.c handler.c module.c intercept.c)
target_link_libraries(lotto-task_velocity.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-task_velocity SHARED)
target_link_libraries(lotto-task_velocity PRIVATE lotto-task_velocity.o)
