#include <unistd.h>

#include <sys/types.h>
#include <vsync/atomic/core.h>
#include <vsync/spinlock/caslock.h>

#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/modules/uafcheck.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/memmgr_impl.h>
#include <lotto/sys/real.h>

static caslock_t _lock;
static pid_t _owner;
static uafc_t *_uc;

#define FORWARD(F, ...) uafc_##F(_uc, __VA_ARGS__)

static void
_uaf_mem_init(void)
{
    if (_uc != NULL)
        return;
    _uc = uafc_init(real_func(memmgr_alloc_name, NULL),
                    real_func(memmgr_aligned_alloc_name, NULL),
                    real_func(memmgr_free_name, NULL),
                    real_func(memmgr_realloc_name, NULL));
    ASSERT(_uc);
}

static bool
_uaf_lock(void)
{
    caslock_acquire(&_lock);
    _owner = 1;
    return true;
}

static void
_uaf_unlock(void)
{
    _owner = 0;
    caslock_release(&_lock);
}

static bool
_uaf_nested(void)
{
    _uaf_mem_init();
    return _owner;
}

void
uaf_fini()
{
    while (!_uaf_lock())
        ;
    logger_debugf("Finalize user memory\n");
    uafc_fini(_uc);
    _uaf_unlock();
}


/* *****************************************************************************
 * public interface
 * ****************************************************************************/

void *
memmgr_alloc(size_t n)
{
    if (_uaf_nested() || !_uaf_lock())
        return _uc->alloc(n);
    void *ptr = FORWARD(alloc, n);
    _uaf_unlock();
    return ptr;
}

void *
memmgr_realloc(void *p, size_t n)
{
    if (_uaf_nested() || !_uaf_lock())
        return _uc->realloc(p, n);
    void *ptr = FORWARD(realloc, p, n);
    _uaf_unlock();
    return ptr;
}

void *
memmgr_aligned_alloc(size_t alignment, size_t size)
{
    if (_uaf_nested() || !_uaf_lock())
        return _uc->aligned_alloc(alignment, size);
    void *ptr = FORWARD(aligned_alloc, alignment, size);
    _uaf_unlock();
    return ptr;
}

void
memmgr_free(void *p)
{
    if (_uaf_nested() || !_uaf_lock())
        return _uc->free(p);
    FORWARD(free, p);
    _uaf_unlock();
}
