#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/mutex.h>
#include <lotto/runtime/intercept.h>

static inline void
capture_simple(const char *func, category_t cat, void *addr)
{
    if (intercept_capture != NULL) {
        context_t *c = ctx();
        c->func      = func;
        c->cat       = cat;
        c->args[0]   = arg_ptr(addr);
        intercept_capture(c);
    }
}

void
_lotto_mutex_acquire_named(const char *func, void *addr)
{
    capture_simple(func, CAT_MUTEX_ACQUIRE, addr);
}

void
_lotto_mutex_acquire(void *addr)
{
    _lotto_mutex_acquire_named(__FUNCTION__, addr);
}

int
_lotto_mutex_tryacquire_named(const char *func, void *addr)
{
    context_t *c = ctx();
    c->func      = func;
    c->cat       = CAT_MUTEX_TRYACQUIRE;
    c->args[0]   = arg_ptr(addr);
    if (intercept_capture != NULL) {
        intercept_capture(c);
    }
    return (int)c->args[1].value.u8;
}

int
_lotto_mutex_tryacquire(void *addr)
{
    return _lotto_mutex_tryacquire_named(__FUNCTION__, addr);
}

void
_lotto_mutex_release_named(const char *func, void *addr)
{
    capture_simple(func, CAT_MUTEX_RELEASE, addr);
}

void
_lotto_mutex_release(void *addr)
{
    _lotto_mutex_release_named(__FUNCTION__, addr);
}
