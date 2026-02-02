#include <stddef.h>
#include <stdlib.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/memmgr_impl.h>
#include <lotto/sys/real.h>

#define FORWARD(F, ...) REAL_NAME(F)(__VA_ARGS__)

REAL_DECL(void *, malloc, size_t n);
REAL_DECL(void *, realloc, void *p, size_t n);
REAL_DECL(void, free, void *p);
REAL_DECL(int, posix_memalign, void **p, size_t a, size_t s);

static void
memmgr_init()
{
    if (REAL_NAME(malloc) != NULL)
        return;

    REAL_NAME(malloc)         = real_func("malloc", 0);
    REAL_NAME(realloc)        = real_func("realloc", 0);
    REAL_NAME(free)           = real_func("free", 0);
    REAL_NAME(posix_memalign) = real_func("posix_memalign", 0);

    ASSERT(REAL_NAME(malloc) != NULL);
    ASSERT(REAL_NAME(realloc) != NULL);
    ASSERT(REAL_NAME(free) != NULL);
    ASSERT(REAL_NAME(posix_memalign) != NULL);
}

void
memmgr_fini()
{
}

void *
memmgr_alloc(size_t size)
{
    memmgr_init();
    return FORWARD(malloc, size);
}

void *
memmgr_aligned_alloc(size_t alignment, size_t size)
{
    memmgr_init();
    void *ptr;
    return FORWARD(posix_memalign, &ptr, alignment, size) == 0 ? ptr : NULL;
}

void *
memmgr_realloc(void *ptr, size_t size)
{
    memmgr_init();
    return FORWARD(realloc, ptr, size);
}

void
memmgr_free(void *ptr)
{
    memmgr_init();
    FORWARD(free, ptr);
}
