// clang-format off
// XFAIL: *
// RUN: exit 1
// RUN: %lotto %sc --delayed-functions foo:1:0 --delayed-calls 2 --sc-exploration-rounds 50 -r 100 -- %b 2>&1
// clang-format on

#include <pthread.h>
#include <stdatomic.h>

atomic_int x = 0;

int
foo()
{
    // The compiler detects x is an atomic variable and treates this as
    //      __atomic_add_fetch(&x, 1, __ATOMIC_SEQ_CST);
    // This forces function foo to behave atomically even in not atomic mode
    return x++;
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
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
