// clang-format off
// RUN: %lotto %stress -r 50 -- %b
// clang-format on

#include <pthread.h>
#include <stdbool.h>

#define N_READERS 7
#define N_WRITERS 3

pthread_rwlock_t l;
pthread_t tr[N_READERS];
pthread_t tw[N_WRITERS];

void *
reader(void *arg)
{
    (void)arg;
    int err1, err2;
    err1 = pthread_rwlock_tryrdlock(&l);
    err2 = pthread_rwlock_tryrdlock(&l);
    if (!err1)
        pthread_rwlock_unlock(&l);
    if (!err2)
        pthread_rwlock_unlock(&l);
    return NULL;
}

void *
writer(void *arg)
{
    (void)arg;
    int err1;
    err1 = pthread_rwlock_trywrlock(&l);
    if (!err1)
        pthread_rwlock_unlock(&l);
    return NULL;
}

int
main()
{
    pthread_rwlock_init(&l, NULL);
    for (int i = 0; i < N_READERS; ++i) {
        pthread_create(&tr[i], NULL, reader, NULL);
    }
    for (int i = 0; i < N_WRITERS; ++i) {
        pthread_create(&tw[i], NULL, writer, NULL);
    }
    for (int i = 0; i < N_READERS; ++i) {
        pthread_join(tr[i], NULL);
    }
    for (int i = 0; i < N_WRITERS; ++i) {
        pthread_join(tw[i], NULL);
    }
    pthread_rwlock_destroy(&l);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
