/*
 */
// clang-format off
#include <vsync/thread/mutex.h>
#include <vsync/thread/cond.h>
// clang-format on

#include <lotto/base/context.h>
#include <lotto/order.h>
#include <lotto/runtime/intercept.h>

static uint64_t next_order = 1;
static vmutex_t verifier_mutex;
static vcond_t verifier_cnd;

void
lotto_order(uint64_t order)
{
    intercept_before_call(ctx(.func = __FUNCTION__, .cat = CAT_CALL,
                              .args = {arg(uint64_t, order)}));

    vmutex_acquire(&verifier_mutex);

    while (order != next_order) {
        vcond_signal(&verifier_cnd);
        vcond_wait(&verifier_cnd, &verifier_mutex);
    }

    intercept_after_call(__FUNCTION__);

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
