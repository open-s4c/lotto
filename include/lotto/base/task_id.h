#ifndef LOTTO_TASK_ID_H
#define LOTTO_TASK_ID_H
#include <stdint.h>

typedef uint64_t task_id;
#define NO_TASK  ((task_id)0)
#define ANY_TASK (~NO_TASK)

task_id get_task_count(void);

#endif
