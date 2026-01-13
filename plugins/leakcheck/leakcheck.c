/*
 */

#ifndef DISABLE_EXECINFO
    #include <execinfo.h>
#endif
#include <stdlib.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/memmgr_impl.h>
#include <lotto/sys/real.h>
#include <lotto/sys/string.h>
#include <vsync/spinlock/caslock.h>

#define MAX_FRAMES 10

struct alloc {
    void *btrace[MAX_FRAMES];
    int btsize;
    struct alloc *next;
    size_t size;
    char data[0];
};

typedef struct leakcheck {
    caslock_t lock;
    struct alloc list;
} leakcheck_t;

static leakcheck_t _lc;

REAL_DECL(void *, memmgr_alloc, size_t n);
REAL_DECL(void *, memmgr_realloc, void *p, size_t n);
REAL_DECL(void, memmgr_free, void *p);

#define FORWARD(F, ...) REAL_NAME(F)(__VA_ARGS__)

static void
memmgr_init()
{
    if (REAL_NAME(memmgr_alloc) != NULL)
        return;

    REAL_NAME(memmgr_alloc)   = real_func(memmgr_alloc_name, 0);
    REAL_NAME(memmgr_realloc) = real_func(memmgr_realloc_name, 0);
    REAL_NAME(memmgr_free)    = real_func(memmgr_free_name, 0);

    ASSERT(REAL_NAME(memmgr_alloc) != NULL);
    ASSERT(REAL_NAME(memmgr_realloc) != NULL);
    ASSERT(REAL_NAME(memmgr_free) != NULL);

    caslock_init(&_lc.lock);
    _lc.list = (struct alloc){};
}

void *
memmgr_alloc(size_t size)
{
    memmgr_init();
    struct alloc *a = FORWARD(memmgr_alloc, size + sizeof(struct alloc));
    ASSERT(a);

#ifndef DISABLE_EXECINFO
    a->btsize = backtrace((void **)&a->btrace, MAX_FRAMES);
#else
    a->btsize = 0;
#endif
    a->size = size;

    caslock_acquire(&_lc.lock);
    a->next       = _lc.list.next;
    _lc.list.next = a;
    caslock_release(&_lc.lock);
    return a->data;
}

void *
memmgr_aligned_alloc(size_t alignment, size_t size)
{
    ASSERT(0 && "not implemented");
    return 0;
}

void
memmgr_free(void *ptr)
{
    memmgr_init();

    size_t sz          = sizeof(struct alloc);
    struct alloc *a    = (struct alloc *)((char *)ptr - sz);
    struct alloc *prev = &_lc.list;

    caslock_acquire(&_lc.lock);
    while (prev->next != a)
        prev = prev->next;

    ASSERT(prev->next == a);
    prev->next = a->next;
    caslock_release(&_lc.lock);

    FORWARD(memmgr_free, a);
}

void *
memmgr_realloc(void *ptr, size_t size)
{
    struct alloc *a = memmgr_alloc(size);
    if (!ptr || !a)
        return a->data;

    size_t sz           = sizeof(struct alloc);
    struct alloc *old_a = (struct alloc *)((char *)ptr - sz);
    size                = old_a->size < size ? old_a->size : size;
    sys_memcpy(a->data, ptr, size);

    memmgr_free(ptr);
    return a->data;
}

void
memmgr_fini()
{
    struct alloc *n = &_lc.list;

    caslock_acquire(&_lc.lock);

    if (_lc.list.next)
        logger_printf("\n\n");

    while ((n = _lc.list.next) != NULL) {
        _lc.list.next = n->next;

        logger_printf("Memory leak: addr=%p size=%lu\n", n->data, n->size);
#ifndef DISABLE_EXECINFO
        char **strs = backtrace_symbols(n->btrace, n->btsize);
        for (int i = 0; i < n->btsize; i++) {
            logger_printf("  %s\n", strs[i]);
        }
        logger_printf("\n");
        free(strs);
#endif
        FORWARD(memmgr_free, n);
    }
    caslock_release(&_lc.lock);
}
