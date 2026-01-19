// clang-format off
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress -v -s random --check-deadlock -- %b)
// RUN: %lotto %inflex -s random --handler-lock enable -r 100
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

atomic_bool at1 = true;
atomic_bool at2 = true;

static void *
thread1(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    printf("Thread 1 acquired mutex2\n");
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

static void *
thread2(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&mutex2);
    printf("Thread 2 acquired mutex2\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 2 acquired mutex1\n");
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int
main()
{
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
