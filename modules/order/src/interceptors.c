// clang-format off
#include <vsync/thread/mutex.h>
#include <vsync/thread/cond.h>
// clang-format on

#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/modules/order/events.h>
#include <lotto/order.h>

static uint64_t next_order = 1;
static vmutex_t verifier_mutex;
static vcond_t verifier_cnd;

typedef struct {
    const char *func;
    uint64_t order;
} order_event_t;

PS_ADVERTISE_TYPE(EVENT_ORDER)

void
lotto_order(uint64_t order)
{
    order_event_t ev = {
        .func  = __FUNCTION__,
        .order = order,
    };
    PS_PUBLISH(INTERCEPT_BEFORE, EVENT_ORDER, &ev, 0);

    vmutex_acquire(&verifier_mutex);

    while (order != next_order) {
        vcond_signal(&verifier_cnd);
        vcond_wait(&verifier_cnd, &verifier_mutex);
    }

    PS_PUBLISH(INTERCEPT_AFTER, EVENT_ORDER, &ev, 0);

    next_order++;

    vcond_signal(&verifier_cnd);
    vmutex_release(&verifier_mutex);
}

void
lotto_order_cond(bool cond, uint64_t order)
{
    if (!cond)
        return;

    lotto_order(order);
}
