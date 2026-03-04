#include <lotto/mutex.h>
#include <lotto/runtime/intercept.h>

void
_lotto_mutex_acquire_named(const char *func, void *addr, const void *pc)
{
    intercept_capture(ctx_pc(.pc = (uintptr_t)pc, .func = func,
                             .cat  = CAT_MUTEX_ACQUIRE,
                             .args = {arg_ptr(addr)}));
}
void
_lotto_mutex_acquire(void *addr, const void *pc)
{
    _lotto_mutex_acquire_named(__FUNCTION__, addr, pc);
}


int
_lotto_mutex_tryacquire_named(const char *func, void *addr, const void *pc)
{
    context_t *ctx =
        ctx_pc(.pc = (uintptr_t)pc, .func = func, .cat = CAT_MUTEX_TRYACQUIRE,
               .args = {arg_ptr(addr)});
    intercept_capture(ctx);
    return ctx->args[1].value.u8;
}

int
_lotto_mutex_tryacquire(void *addr, const void *pc)
{
    return _lotto_mutex_tryacquire_named(__FUNCTION__, addr, pc);
}

void
_lotto_mutex_release_named(const char *func, void *addr, const void *pc)
{
    intercept_capture(ctx_pc(.pc = (uintptr_t)pc, .func = func,
                             .cat  = CAT_MUTEX_RELEASE,
                             .args = {arg_ptr(addr)}));
}

void
_lotto_mutex_release(void *addr, const void *pc)
{
    _lotto_mutex_release_named(__FUNCTION__, addr, pc);
}
