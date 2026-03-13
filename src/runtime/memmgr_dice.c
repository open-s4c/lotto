#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/memmgr_impl.h>

#include <dice/mempool.h>

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
    return mempool_alloc(size);
}

void *
memmgr_aligned_alloc(size_t alignment, size_t size)
{
    return mempool_aligned_alloc(alignment, size);
}

void *
memmgr_realloc(void *ptr, size_t size)
{
    return mempool_realloc(ptr, size);
}

void
memmgr_free(void *ptr)
{
    mempool_free(ptr);
}
