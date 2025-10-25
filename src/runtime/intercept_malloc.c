#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/real.h>

static _Thread_local bool g_in_allocator;

static inline bool
enter_interceptor(void)
{
    if (g_in_allocator) {
        return false;
    }
    g_in_allocator = true;
    return true;
}

static inline void
leave_interceptor(bool active)
{
    if (active) {
        g_in_allocator = false;
    }
}

static inline context_t *
make_ctx(const char *func, category_t cat)
{
    context_t *c = ctx();
    c->func      = func;
    c->cat       = cat;
    return c;
}

void *
malloc(size_t size)
{
    REAL_LIBC_INIT(void *, malloc, size_t);
    bool active     = enter_interceptor();
    context_t *c    = NULL;
    if (active && intercept_before_call != NULL) {
        c          = make_ctx(__FUNCTION__, CAT_CALL);
        c->args[0] = arg(size_t, size);
        intercept_before_call(c);
    }

    void *ptr = REAL(malloc, size);

    if (active && intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    leave_interceptor(active);
    return ptr;
}

void *
calloc(size_t nmemb, size_t size)
{
    REAL_LIBC_INIT(void *, calloc, size_t, size_t);
    bool active     = enter_interceptor();
    context_t *c    = NULL;
    if (active && intercept_before_call != NULL) {
        c          = make_ctx(__FUNCTION__, CAT_CALL);
        c->args[0] = arg(size_t, nmemb);
        c->args[1] = arg(size_t, size);
        intercept_before_call(c);
    }

    void *ptr = REAL(calloc, nmemb, size);

    if (active && intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    leave_interceptor(active);
    return ptr;
}

void *
realloc(void *ptr, size_t size)
{
    REAL_LIBC_INIT(void *, realloc, void *, size_t);
    bool active     = enter_interceptor();
    context_t *c    = NULL;
    if (active && intercept_before_call != NULL) {
        c          = make_ctx(__FUNCTION__, CAT_CALL);
        c->args[0] = arg_ptr(ptr);
        c->args[1] = arg(size_t, size);
        intercept_before_call(c);
    }

    void *ret = REAL(realloc, ptr, size);

    if (active && intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    leave_interceptor(active);
    return ret;
}

void
free(void *ptr)
{
    REAL_LIBC_INIT(void, free, void *);
    bool active     = enter_interceptor();
    context_t *c    = NULL;
    if (active && intercept_before_call != NULL) {
        c          = make_ctx(__FUNCTION__, CAT_CALL);
        c->args[0] = arg_ptr(ptr);
        intercept_before_call(c);
    }

    REAL(free, ptr);

    if (active && intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    leave_interceptor(active);
}

int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
    REAL_LIBC_INIT(int, posix_memalign, void **, size_t, size_t);
    bool active     = enter_interceptor();
    context_t *c    = NULL;
    if (active && intercept_before_call != NULL) {
        c          = make_ctx(__FUNCTION__, CAT_CALL);
        c->args[0] = arg_ptr(memptr);
        c->args[1] = arg(size_t, alignment);
        c->args[2] = arg(size_t, size);
        intercept_before_call(c);
    }

    int ret = REAL(posix_memalign, memptr, alignment, size);

    if (active && intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    leave_interceptor(active);
    return ret;
}

void *
aligned_alloc(size_t alignment, size_t size)
{
    REAL_LIBC_INIT(void *, aligned_alloc, size_t, size_t);
    bool active     = enter_interceptor();
    context_t *c    = NULL;
    if (active && intercept_before_call != NULL) {
        c          = make_ctx(__FUNCTION__, CAT_CALL);
        c->args[0] = arg(size_t, alignment);
        c->args[1] = arg(size_t, size);
        intercept_before_call(c);
    }

    void *ptr = NULL;
    if (REAL_NAME(aligned_alloc) != NULL) {
        ptr = REAL(aligned_alloc, alignment, size);
    } else {
        int err = posix_memalign(&ptr, alignment, size);
        if (err != 0) {
            ptr = NULL;
        }
    }

    if (active && intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    leave_interceptor(active);
    return ptr;
}
