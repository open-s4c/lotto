/**
 * @file trace.c
 * @brief Available module trace helper definitions.
 */
#include <lotto/engine/statemgr.h>
#include <lotto/modules/available/state.h>
#include <lotto/modules/available/trace.h>
#include <lotto/sys/stdlib.h>

tidset_t *
trace_alternative_tasks(trace_t *trace)
{
    record_t *record = trace_last(trace);
    if (record == NULL)
        return NULL;

    tidset_t *tidset = sys_malloc(sizeof(tidset_t));
    tidset_init(tidset);
    statemgr_unmarshal(record->data, STATE_TYPE_PERSISTENT, false);
    tidset_copy(tidset, get_available_tasks());
    tidset_remove(tidset, record->id);
    return tidset;
}
