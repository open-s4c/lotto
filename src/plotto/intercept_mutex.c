/*
 */

#include <lotto/mutex.h>
#include <lotto/runtime/intercept.h>

void
_lotto_mutex_acquire_named(const char *func, void *addr)
{
    intercept_capture(
        ctx(.func = func, .cat = CAT_MUTEX_ACQUIRE, .args = {arg_ptr(addr)}));
}
void
_lotto_mutex_acquire(void *addr)
{
    _lotto_mutex_acquire_named(__FUNCTION__, addr);
}


int
_lotto_mutex_tryacquire_named(const char *func, void *addr)
{
    context_t *ctx =
        ctx(.func = func, .cat = CAT_MUTEX_TRYACQUIRE, .args = {arg_ptr(addr)});
    intercept_capture(ctx);
    return ctx->args[1].value.u8;
}

int
_lotto_mutex_tryacquire(void *addr)
{
    return _lotto_mutex_tryacquire_named(__FUNCTION__, addr);
}

void
_lotto_mutex_release_named(const char *func, void *addr)
{
    intercept_capture(
        ctx(.func = func, .cat = CAT_MUTEX_RELEASE, .args = {arg_ptr(addr)}));
}

void
_lotto_mutex_release(void *addr)
{
    _lotto_mutex_release_named(__FUNCTION__, addr);
}
