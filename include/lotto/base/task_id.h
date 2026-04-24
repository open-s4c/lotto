/**
 * @file task_id.h
 * @brief Base declarations for task id.
 */
#ifndef LOTTO_TASK_ID_H
#define LOTTO_TASK_ID_H

#include <stdint.h>

typedef uint64_t task_id;
#define NO_TASK  ((task_id)0)
#define ANY_TASK (~NO_TASK)

#endif
