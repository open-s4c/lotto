/**
 * @file events.h
 * @brief Rwlock semantic ingress event payloads.
 */
#ifndef LOTTO_MODULES_RWLOCK_EVENTS_H
#define LOTTO_MODULES_RWLOCK_EVENTS_H

#include <time.h>

#include <dice/events/pthread.h>

struct rwlock_rdlock_event {
    const void *pc;
    const char *func;
    void *lock;
};

struct rwlock_wrlock_event {
    const void *pc;
    const char *func;
    void *lock;
};

struct rwlock_unlock_event {
    const void *pc;
    const char *func;
    void *lock;
};

struct rwlock_tryrdlock_event {
    const void *pc;
    const char *func;
    void *lock;
    int ret;
};

struct rwlock_trywrlock_event {
    const void *pc;
    const char *func;
    void *lock;
    int ret;
};

struct rwlock_timedrdlock_event {
    const void *pc;
    const char *func;
    void *lock;
    const struct timespec *abstime;
};

struct rwlock_timedwrlock_event {
    const void *pc;
    const char *func;
    void *lock;
    const struct timespec *abstime;
};

#endif
