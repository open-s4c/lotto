#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/arg.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger.h>

#define EVENT_SCHED_YIELD 105
PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SCHED_YIELD, {
    context_t *c = ctx(.func = "sched_yield", .cat = CAT_USER_YIELD);
    intercept_capture(c);
    return PS_OK;
})

PS_ADVERTISE_TYPE(EVENT_SCHED_YIELD)
INTERPOSE(int, sched_yield, void)
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SCHED_YIELD, NULL, 0);
    return 0;
}
