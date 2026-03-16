#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <dice/interpose.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/memmgr_impl.h>

#define FORWARD(F, ...) REAL_NAME(F)(__VA_ARGS__)

REAL_DECL(void *, malloc, size_t n);
REAL_DECL(void *, realloc, void *p, size_t n);
REAL_DECL(void, free, void *p);
REAL_DECL(int, posix_memalign, void **p, size_t a, size_t s);
REAL_DECL(int, aligned_alloc, size_t a, size_t s);

static void
memmgr_init()
{
}

void
memmgr_fini()
{
}

void *
memmgr_alloc(size_t size)
{
    return REAL(malloc, size);
}

void *
memmgr_aligned_alloc(size_t alignment, size_t size)
{
    void *ptr;
    int err = REAL(posix_memalign, &ptr, alignment, size);
    return err == 0 ? ptr : NULL;
}

void *
memmgr_realloc(void *ptr, size_t size)
{
    return REAL(realloc, ptr, size);
}

void
memmgr_free(void *ptr)
{
    return REAL(free, ptr);
}
