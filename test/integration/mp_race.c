// clang-format off
// RUN: (! %lotto %stress --stable-address-method MASK -- %b 2>&1) | filecheck %s
// CHECK: assert failed {{.*}}/mp_race.c:{{[0-9]+}}: data == 1
//
// RUN: (! %lotto %record --handler-race-strict --stable-address-method MASK -- %b 2>&1) | filecheck %s --check-prefix=HANDLER
// HANDLER: {{\[.*handler_race.c\]}} Data race detected at addr: {{.*}}
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>

int data;
volatile int ctrl;

static void *
run_alice(void *arg)
{
    (void)arg;
    ctrl = 1;
    data = 1;

    return NULL;
}

static void *
run_bob(void *arg)
{
    (void)arg;
    if (ctrl)
        assert(data == 1);

    return NULL;
}

int
main()
{
    pthread_t alice, bob;
    pthread_create(&bob, 0, run_bob, 0);
    pthread_create(&alice, 0, run_alice, 0);
    pthread_join(alice, 0);
    pthread_join(bob, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
