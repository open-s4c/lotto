// clang-format off
// RUN: %lotto %stress -r 100 --delayed-functions foo:0:0 --delayed-calls 2 --delayed-atomic -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x = 0;

void
foo()
{
    x = x + 1;
}

void *
thread()
{
    foo();
    return NULL;
}

int
main()
{
    pthread_t thread1, thread2;
    pthread_create(&thread1, 0, thread, 0);
    pthread_create(&thread2, 0, thread, 0);
    pthread_join(thread1, 0);
    pthread_join(thread2, 0);
    assert(x == 2);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
