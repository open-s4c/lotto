/*
 */
#include <lotto/base/task_id.h>
#include <lotto/sys/assert.h>
#include <vsync/atomic.h>
#include <vsync/atomic/dispatch.h>

static vatomic64_t task_count = VATOMIC_INIT(NO_TASK);

task_id
next_task_id(void)
{
    /* Invariant: there should be only of task creating a task_id at a time
     * to check that we use cmpxchg */

    uint64_t id  = vatomic_read_rlx(&task_count);
    uint64_t old = vatomic_cmpxchg_rlx(&task_count, id, id + 1);
    ASSERT(old == id && "at most one task creating and id");
    return id + 1;
}

task_id
get_task_count(void)
{
    return vatomic_read_rlx(&task_count);
}
