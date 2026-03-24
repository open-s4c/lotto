// clang-format off
// RUN: %lotto %run -- %b
// clang-format on

#include <pthread.h>

void *
thread()
{
    return NULL;
}

int
main()
{
    pthread_t thread1;
    pthread_create(&thread1, 0, thread, 0);
    pthread_join(thread1, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
