#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/modules/deadlock/events.h>
#include <lotto/rsrc_deadlock.h>

PS_ADVERTISE_TYPE(EVENT_RSRC_ACQUIRING)
PS_ADVERTISE_TYPE(EVENT_RSRC_RELEASED)

void
intercept_rsrc_acquiring(void *addr)
{
    rsrc_event_t ev = {.addr = addr};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RSRC_ACQUIRING, &ev, 0);
}

void
intercept_rsrc_released(void *addr)
{
    rsrc_event_t ev = {.addr = addr};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RSRC_RELEASED, &ev, 0);
}

void
_lotto_rsrc_acquiring(void *addr)
{
    rsrc_event_t ev = {.addr = addr};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RSRC_ACQUIRING, &ev, 0);
}

void
_lotto_rsrc_released(void *addr)
{
    rsrc_event_t ev = {.addr = addr};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RSRC_RELEASED, &ev, 0);
}
