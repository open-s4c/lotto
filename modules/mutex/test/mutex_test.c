// clang-format off
// RUN: %lotto %stress -r 50 -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>

#define N_THREADS 10

pthread_mutex_t m1;

void *
thread(void *arg)
{
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    return NULL;
}

int
main()
{
    pthread_t t[N_THREADS];
    pthread_mutex_init(&m1, 0);
    for (int i = 0; i < N_THREADS; ++i) {
        pthread_create(&t[i], 0, thread, 0);
    }
    for (int i = 0; i < N_THREADS; ++i) {
        pthread_join(t[i], 0);
    }
    pthread_mutex_destroy(&m1);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
