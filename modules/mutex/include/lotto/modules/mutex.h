#ifndef LOTTO_HANDLER_MUTEX_H
#define LOTTO_HANDLER_MUTEX_H

#include <lotto/base/task_id.h>

task_id mutex_owner(void *addr);

#endif
