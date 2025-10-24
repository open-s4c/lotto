#ifndef MUTEX_H_
#define MUTEX_H_

#include <pthread.h>

#include <lotto/qemu/mutex.h>

#ifdef RECURSIVE
    #include <vsync/spinlock/rec_spinlock.h>

static __thread uint32_t _tid;
static vatomic32_t _ids;

static uint32_t
tid(void)
{
    if (_tid == 0)
        _tid = vatomic32_inc_get(&_ids);
    return _tid;
}

int
pthread_mutex_lock(pthread_mutex_t *arg)
{
    rec_spinlock_t *m = (rec_spinlock_t *)arg;
    lotto_lock_acquiring(arg);
    rec_spinlock_acquire(m, tid());
    return 0;
}


int
pthread_mutex_unlock(pthread_mutex_t *arg)
{
    rec_spinlock_t *m = (rec_spinlock_t *)arg;
    rec_spinlock_release(m);
    lotto_lock_released(arg);
    return 0;
}

int
pthread_mutex_trylock(pthread_mutex_t *arg)
{
    rec_spinlock_t *m = (rec_spinlock_t *)arg;
    if (rec_spinlock_tryacquire(m, tid())) {
        lotto_lock_acquiring(arg);
        return 0;
    }
    return 1;
}
#else
    #include <vsync/thread/mutex.h>

static inline bool
mutex_tryacquire(vmutex_t *m)
{
    return vatomic32_cmpxchg_acq(m, 0, 1) == 0;
}

int
pthread_mutex_lock(pthread_mutex_t *arg)
{
    vmutex_t *m = (vmutex_t *)arg;
    lotto_lock_acquiring(arg);
    vmutex_acquire(m);
    return 0;
}


int
pthread_mutex_unlock(pthread_mutex_t *arg)
{
    vmutex_t *m = (vmutex_t *)arg;
    vmutex_release(m);
    lotto_lock_released(arg);
    return 0;
}

int
pthread_mutex_trylock(pthread_mutex_t *arg)
{
    vmutex_t *m = (vmutex_t *)arg;
    if (mutex_tryacquire(m)) {
        lotto_lock_acquiring(arg);
        return 0; // because phtread_mutex_trylock returns 0 on success
    }
    return 1;
}
#endif

#endif
