// clang-format off
// RUN: %lotto %run -- %b
// clang-format on

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

pthread_cond_t c;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
struct timespec to;

void *
t()
{
    pthread_mutex_lock(&m);
    assert(pthread_cond_timedwait(&c, &m, &to) == ETIMEDOUT);
    pthread_mutex_unlock(&m);
    return NULL;
}

int
main()
{
    pthread_t thread;
    pthread_create(&thread, 0, t, 0);
    clock_gettime(CLOCK_MONOTONIC, &to);
    to.tv_sec += 3600;
    pthread_mutex_lock(&m);
    assert(pthread_cond_timedwait(&c, &m, &to) == ETIMEDOUT);
    pthread_mutex_unlock(&m);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
