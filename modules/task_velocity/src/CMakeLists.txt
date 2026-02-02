add_library(lotto-task_velocity.o OBJECT state.c handler.c module.c intercept.c)
target_link_libraries(lotto-task_velocity.o PRIVATE lotto.h lotto-src.h dice.h)

add_library(lotto-task_velocity SHARED)
target_link_libraries(lotto-task_velocity PRIVATE lotto-task_velocity.o)

add_library(lotto-driver-task_velocity SHARED state.c flags.c module.c)
target_link_libraries(lotto-driver-task_velocity PRIVATE lotto.h lotto-src.h dice.h)
target_link_libraries(lotto-driver PRIVATE lotto-driver-task_velocity)
