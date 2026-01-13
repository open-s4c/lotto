/*
 */
#include <stddef.h>

#include <lotto/base/log.h>
#include <lotto/base/task_id.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/memory.h>

void
log_init(log_t *log)
{
    ASSERT(log);
    *log = (log_t){.m = MARSHABLE_LOG};
}

bool
log_equals(const log_t *log1, const log_t *log2)
{
    ASSERT(log1 != NULL && log2 != NULL);
    return log1->tid == log2->tid && log1->id == log2->id &&
           log1->data == log2->data;
}

void
log_print(const marshable_t *m)
{
    log_t *l = (log_t *)m;
    logger_infof("[%lu, %lu, 0x%lx]\n", l->tid, l->id, l->data);
}
