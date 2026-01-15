// clang-format off
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress --stable-address-method MASK -- %b 2>&1) | %check %s --check-prefix=BUG
// RUN: %lotto %inflex -r 50 &>/dev/null
// RUN: %lotto %debug --file-filter="libvsync" <<< $'\n'run-replay-lotto | %check %s --check-prefix=LOC
// BUG: assert failed {{.*}}/debug-replay-end.c:{{[0-9]+}}: data == 1
// LOC: assert(data == 1);
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int data;
atomic_int ctrl;

void *
run_alice(void *arg)
{
    ctrl = 1;
    data = 1;

    return NULL;
}

static void *
run_bob(void *arg)
{
    if (ctrl) {
        assert(data == 1);
    }

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
