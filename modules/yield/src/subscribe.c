#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <dice/chains/capture.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/arg.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/modules/yield/events.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SCHED_YIELD, {
    context_t *c = ctx(.func = "sched_yield", .cat = CAT_USER_YIELD);
    intercept_capture(c);
    return PS_OK;
})
