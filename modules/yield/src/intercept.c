#include <dice/chains/intercept.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>

#define EVENT_SCHED_YIELD 105

PS_ADVERTISE_TYPE(EVENT_SCHED_YIELD)
INTERPOSE(int, sched_yield, void)
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SCHED_YIELD, NULL, 0);
    return 0;
}
