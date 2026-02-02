#include <unistd.h>

#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/pubsub_interface.h>
#include <lotto/runtime/intercept.h>
#include <lotto/runtime/mediator.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/memmgr_runtime.h>
#include <lotto/sys/memmgr_user.h>
#include <lotto/sys/mempool.h>
#include <lotto/sys/real.h>
#include <lotto/sys/string.h>
#include <sys/types.h>
#include <vsync/atomic/core.h>
#include <vsync/spinlock/caslock.h>

#define F(X) X
#define SHOULD_REDIRECT_RUNTIME                                                \
    (lotto_intercept_initialized() && mediator_in_capture(get_mediator(false)))
#define REDIRECT(F, ...) memmgr_user_##F(__VA_ARGS__)

/* *****************************************************************************
 * internal state and interface
 * ****************************************************************************/

static caslock_t _lock;
static pid_t _owner;

static void
_intercept_posix_mem_real(void)
{
}

static void
_intercept_posix_mem_init(void)
{
    _intercept_posix_mem_real();
}

static bool
_intercept_posix_mem_lock(void)
{
    pid_t tid = gettid();
    if (_owner == tid) {
        _intercept_posix_mem_real();
        return false;
    }

    caslock_acquire(&_lock);
    _intercept_posix_mem_init();
    _owner = tid;

    return true;
}
static void
_intercept_posix_mem_unlock(void)
{
    _owner = 0;
    caslock_release(&_lock);
}

/* *****************************************************************************
 * lotto_mempool
 * ****************************************************************************/

#define LOTTO_MEMPOOL_SIZE       ((size_t)1024 * 1024 * 1024)
#define SHOULD_USE_LOTTO_MEMPOOL (_recursive > 0)
#define IN_LOTTO_MEMPOOL(p)                                                    \
    ((char *)(p) >= _pool && (char *)(p) <= _pool + sizeof(_pool))
#define LOTTO_MEMPOOL_REDIRECT(F, ...)                                         \
    lotto_mempool_##F(&_lotto_mempool, __VA_ARGS__)

static uint64_t _recursive;
static bool _malloc_initialized = false;

static char _pool[LOTTO_MEMPOOL_SIZE];
static lotto_mempool_t _lotto_mempool;
static bool _lotto_mempool_initialized = false;

static void
_lotto_mempool_init()
{
    if (_lotto_mempool_initialized) {
        return;
    }
    lotto_mempool_init_static(&_lotto_mempool, _pool, sizeof(_pool));
    _lotto_mempool_initialized = true;
}

/* *****************************************************************************
 * public interface
 * ****************************************************************************/

void *
F(malloc)(size_t n)
{
    void *ptr;
    if (SHOULD_USE_LOTTO_MEMPOOL) {
        _lotto_mempool_init();
        ptr = LOTTO_MEMPOOL_REDIRECT(alloc, n);
    } else {
        while (!_intercept_posix_mem_lock())
            ;
        _recursive++;
        ptr                 = REDIRECT(alloc, n);
        _malloc_initialized = true;
        _recursive--;
        _intercept_posix_mem_unlock();
    }
    return ptr;
}
void *
F(calloc)(size_t n, size_t s)
{
    void *ptr = malloc(n * s);
    if (ptr != NULL) {
        sys_memset(ptr, 0, n * s);
    }
    return ptr;
}
void *
F(realloc)(void *p, size_t n)
{
    void *ptr;
    if (IN_LOTTO_MEMPOOL(p)) {
        ptr = LOTTO_MEMPOOL_REDIRECT(realloc, p, n);
    } else {
        while (!_intercept_posix_mem_lock())
            ;
        ptr = REDIRECT(realloc, p, n);
        _intercept_posix_mem_unlock();
    }
    return ptr;
}
void
F(free)(void *p)
{
    if (p == NULL) {
        return;
    }
    if (IN_LOTTO_MEMPOOL(p)) {
        LOTTO_MEMPOOL_REDIRECT(free, p);
        return;
    }
    while (!_intercept_posix_mem_lock())
        ;
    REDIRECT(free, p);
    _intercept_posix_mem_unlock();
}

int
F(posix_memalign)(void **memptr, size_t alignment, size_t size)
{
    ASSERT(_malloc_initialized);
    if (SHOULD_USE_LOTTO_MEMPOOL) {
        _lotto_mempool_init();
        *memptr = LOTTO_MEMPOOL_REDIRECT(aligned_alloc, alignment, size);
    } else {
        while (!_intercept_posix_mem_lock())
            ;
        *memptr = REDIRECT(aligned_alloc, alignment, size);
        _intercept_posix_mem_unlock();
    }
    return 0;
}
