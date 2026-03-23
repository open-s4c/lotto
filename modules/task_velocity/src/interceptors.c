#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/modules/task_velocity/events.h>
#include <lotto/sys/assert.h>
#include <lotto/velocity.h>

PS_ADVERTISE_TYPE(EVENT_TASK_VELOCITY)

static void
intercept_task_velocity(int64_t probability)
{
    task_velocity_event_t ev = {.probability = probability};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_TASK_VELOCITY, &ev, 0);
}

void
lotto_task_velocity(int64_t probability)
{
    ASSERT(probability >= LOTTO_TASK_VELOCITY_MIN &&
           probability <= LOTTO_TASK_VELOCITY_MAX);

    intercept_task_velocity(probability);
}
