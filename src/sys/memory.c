/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/sys/logger_block.h>
#include <lotto/sys/memmgr_runtime.h>
#include <lotto/sys/string.h>

void
sys_memory_fini(void)
{
    logger_debugf("Finalize sys_memory\n");
    memmgr_runtime_fini();
}


void *
sys_malloc(size_t s)
{
    return memmgr_runtime_alloc(s);
}

void *
sys_memalign(size_t alignment, size_t size)
{
    return memmgr_runtime_aligned_alloc(alignment, size);
}

void *
sys_calloc(size_t n, size_t s)
{
    void *ptr = memmgr_runtime_alloc(n * s);
    if (ptr)
        sys_memset(ptr, 0, n * s);
    return ptr;
}

void *
sys_realloc(void *p, size_t s)
{
    return memmgr_runtime_realloc(p, s);
}

void
sys_free(void *p)
{
    memmgr_runtime_free(p);
}
