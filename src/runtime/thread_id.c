#include <stdatomic.h>
#include <stdlib.h>

#include <lotto/base/task_id.h>

static atomic_uint_fast64_t g_task_counter = 0;

task_id
next_task_id(void)
{
    return (task_id)(atomic_fetch_add_explicit(&g_task_counter, 1,
                                               memory_order_relaxed) +
                     1);
}

task_id
get_task_count(void)
{
    return (task_id)atomic_load_explicit(&g_task_counter, memory_order_relaxed);
}
