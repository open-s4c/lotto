#include <sched.h>

#include <lotto/runtime/switcher.h>

switcher_status_t
switcher_yield(task_id id, bool (*any_task_filter)(task_id))
{
    (void)id;
    (void)any_task_filter;
    sched_yield();
    return SWITCHER_CONTINUE;
}

void
switcher_wake(task_id id, nanosec_t slack)
{
    (void)id;
    (void)slack;
}

void
switcher_abort(void)
{
}
