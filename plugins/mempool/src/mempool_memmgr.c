/*
 */
#include <errno.h>
#include <limits.h>
#include <mempool.h>
#include <stdlib.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/memmgr_impl.h>
#include <lotto/sys/mempool.h>
#include <lotto/sys/real.h>
#include <lotto/util/macros.h>

#define FORWARD(F, ...) mempool_##F(_mempool, __VA_ARGS__)

static mempool_t *_mempool;

static void
memmgr_init()
{
    if (_mempool != NULL) {
        return;
    }
    const char *size_var = getenv(XSTR(MEMPOOL_SIZE));
    ASSERT(size_var != NULL);

    errno = 0;
    char *endptr;
    uint64_t size = strtoull(size_var, &endptr, 10);

    ASSERT(errno == 0 && "could not parse the size");
    ASSERT(endptr > size_var && "no size provided");
    ASSERT(size > 0);
    _mempool = mempool_init(real_func(memmgr_alloc_name, NULL),
                            real_func(memmgr_free_name, NULL), size);
    ASSERT(_mempool);
}

void *
memmgr_alloc(size_t size)
{
    memmgr_init();
    return FORWARD(alloc, size);
}

void *
memmgr_aligned_alloc(size_t alignment, size_t size)
{
    memmgr_init();
    return FORWARD(aligned_alloc, alignment, size);
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
