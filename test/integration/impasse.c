// clang-format off
// RUN: (! %lotto %stress -- %b 2>&1) | filecheck %s
// CHECK: [{{.*}}] Deadlock detected! (impasse)
// clang-format on
#include <pthread.h>

static void *
thread(void *arg)
{
    pthread_cond_t cond;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    return NULL;
}

int
main()
{
    pthread_t t;
    pthread_create(&t, NULL, thread, NULL);
    pthread_join(t, NULL);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
