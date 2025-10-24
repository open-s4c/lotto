// clang-format off
// RUN: %lotto %stress -r 50 -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>

#include <lotto/region_atomic.h>

volatile int *data_ptr;
volatile int data;

void *
run_alice(void *arg)
{
    lotto_region_atomic_enter();
    data_ptr = &data;
    for (int i = 0; i < 1000; i++) {
        (*data_ptr)++;
    }
    lotto_region_atomic_leave();
    return NULL;
}

void *
run_bob(void *arg)
{
    data_ptr = NULL;
    return NULL;
}

int
main()
{
    pthread_t alice, bob;
    pthread_create(&alice, 0, run_alice, 0);
    pthread_create(&bob, 0, run_bob, 0);
    pthread_join(alice, 0);
    pthread_join(bob, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
