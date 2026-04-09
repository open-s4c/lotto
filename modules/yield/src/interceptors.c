#include <stdbool.h>

#include <dice/chains/intercept.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/yield/events.h>

typedef struct yield_event {
    bool advisory;
} yield_event_t;

void
intercept_yield(bool advisory)
{
    yield_event_t ev = {.advisory = advisory};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_LOTTO_YIELD, &ev, 0);
}

int
lotto_yield(bool advisory)
{
    intercept_yield(advisory);
    return 0;
}

INTERPOSE(int, sched_yield, void)
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SCHED_YIELD, NULL, 0);
    return 0;
}
