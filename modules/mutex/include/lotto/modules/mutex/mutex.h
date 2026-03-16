/**
 * @file mutex.h
 * @brief Mutex module declarations for mutex.
 */
#ifndef LOTTO_MODULES_MUTEX_MUTEX_H
#define LOTTO_MODULES_MUTEX_MUTEX_H

#include <lotto/base/task_id.h>

task_id mutex_owner(void *addr);

#endif
