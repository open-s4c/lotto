#include <dice/chains/intercept.h>
#include <dice/interpose.h>
#include <dice/pubsub.h>
#include <lotto/modules/clock.h>
#include <lotto/modules/clock/events.h>

uint64_t
lotto_clock_read(void)
{
    struct lotto_clock_event ev = {
        .pc  = INTERPOSE_PC,
        .ret = 0,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_LOTTO_CLOCK, &ev, 0);
    return ev.ret;
}
