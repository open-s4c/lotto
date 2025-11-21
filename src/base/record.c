/*
 */
#include <lotto/base/envvar.h>
#include <lotto/base/record.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>


static const char *_record_map[] = {
    [RECORD_NONE] = "NONE",

    /* Main records */
    [RECORD_INFO]  = "INFO",
    [RECORD_SCHED] = "SCHEDULER",
    [RECORD_FORCE] = "FORCE",

    /* Start and end of trace */
    [RECORD_START] = "START",
    [RECORD_EXIT]  = "EXIT",

    /* Configuration update */
    [RECORD_CONFIG] = "CONFIG",

    /* Record placeholder */
    [RECORD_OPAQUE] = "OPAQUE",
};

record_t *
record_alloc(size_t size)
{
    record_t *record = (record_t *)sys_calloc(1, sizeof(record_t) + size);
    ASSERT(record && "record malloc must succeed");
    record->size = size;
    return record;
}

const char *
kind_str(enum record r)
{
    return _record_map[r];
}

record_t *
record_clone(const record_t *record)
{
    ASSERT(record);
    record_t *result = record_alloc(record->size);
    sys_memcpy(result, record, sizeof(record_t) + record->size);
    return result;
}
