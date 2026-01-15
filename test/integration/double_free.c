// clang-format off
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress --memmgr-user=uaf --stable-address-method MASK -- %b 2>&1) | %check %s
// CHECK: SEGFAULT
// clang-format on

#include <pthread.h>
#include <stdlib.h>

char *ptr;

void *
freeer()
{
    ptr = malloc(1);
    free(ptr);
    return NULL;
}

int
main()
{
    pthread_t thread1, thread2;
    pthread_create(&thread1, 0, freeer, 0);
    pthread_create(&thread2, 0, freeer, 0);
    pthread_join(thread1, 0);
    pthread_join(thread2, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
