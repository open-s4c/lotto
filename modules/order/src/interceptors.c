// clang-format off
#if defined(__linux__) && !defined(FUTEX_USERSPACE)
/* Mirror the runtime switcher workaround: force libvsync futex.h to use the
 * external vfutex_wait/wake declarations so this module goes through
 * src/runtime/futex.c and the lotto_futex override path.
 */
#undef __linux__
#include <vsync/thread/internal/futex.h>
#define __linux__
#endif
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

void
lotto_order(uint64_t order)
{
    order_event_t ev = {
        .func  = __FUNCTION__,
        .order = order,
    };
    struct metadata md = {};
    PS_PUBLISH(INTERCEPT_BEFORE, EVENT_ORDER_SET, &ev, &md);

    vmutex_acquire(&verifier_mutex);

    while (order != next_order) {
        vcond_signal(&verifier_cnd);
        vcond_wait(&verifier_cnd, &verifier_mutex);
    }

    PS_PUBLISH(INTERCEPT_AFTER, EVENT_ORDER_SET, &ev, &md);

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
