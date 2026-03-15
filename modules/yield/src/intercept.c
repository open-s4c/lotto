#include <dice/chains/intercept.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/yield/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_SCHED_YIELD)
INTERPOSE(int, sched_yield, void)
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SCHED_YIELD, NULL, 0);
    return 0;
}
