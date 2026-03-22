#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/modules/priority/events.h>
#include <lotto/priority.h>
#include <lotto/runtime/ingress.h>

PS_ADVERTISE_TYPE(EVENT_PRIORITY)

static void
intercept_priority(int64_t priority)
{
    priority_event_t ev = {.priority = priority};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_PRIORITY, &ev, 0);
}

void
lotto_priority(int64_t priority)
{
    intercept_priority(priority);
}

void
lotto_priority_cond(bool cond, int64_t priority)
{
    if (!cond) {
        return;
    }
    lotto_priority(priority);
}

void
lotto_priority_task(task_id task, int64_t priority)
{
    if (get_task_id() != task) {
        return;
    }
    lotto_priority(priority);
}
