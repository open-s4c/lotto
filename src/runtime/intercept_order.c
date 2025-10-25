#include <pthread.h>

#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/order.h>
#include <lotto/runtime/intercept.h>

static pthread_mutex_t g_order_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_order_cond   = PTHREAD_COND_INITIALIZER;
static uint64_t g_next_order         = 1;

void
lotto_order(uint64_t order)
{
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_CALL;
    c->args[0]   = arg(uint64_t, order);
    if (intercept_before_call != NULL) {
        intercept_before_call(c);
    }

    pthread_mutex_lock(&g_order_mutex);
    while (order != g_next_order) {
        pthread_cond_wait(&g_order_cond, &g_order_mutex);
    }
    ++g_next_order;
    pthread_cond_broadcast(&g_order_cond);
    pthread_mutex_unlock(&g_order_mutex);

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
}

void
lotto_order_cond(bool cond, uint64_t order)
{
    if (cond) {
        lotto_order(order);
    }
}
