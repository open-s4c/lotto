/*
 */
#include <lotto/base/cappt.h>

void
cappt_add_any_task_filter(event_t *e, any_task_filter_f f)
{
    ASSERT(e->any_task_filters.n < MAX_ANY_TASK_FILTERS);
    int i = e->any_task_filters.n;
    e->any_task_filters.val[i] = f;
    e->any_task_filters.n++;
}
