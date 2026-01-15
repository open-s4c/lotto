// clang-format off
// UNSUPPORTED: aarch64, clang
// RUN: (! %lotto %stress --stable-address-method MASK -- %b 2>&1) | %check %s --check-prefix=BUG
// RUN: %lotto %inflex -r 30 --inflex-method=le &>/dev/null
// RUN: %lotto %debug <<< $'\n'run-replay-lotto | %check %s --check-prefix=LOC
// BUG: assert failed {{.*}}/inflex_method_{{.*}}.c:{{[0-9]+}}: x != 0b11
// LOC: x++;
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x = 0;

void *
t1()
{
    x++;
    x++;
    return NULL;
}

void *
t2()
{
    x *= 2;
    assert(x != 0b11);
    return NULL;
}

int
main()
{
    pthread_t t1_t, t2_t;

    pthread_create(&t1_t, 0, t1, 0);
    pthread_create(&t2_t, 0, t2, 0);

    pthread_join(t1_t, 0);
    pthread_join(t2_t, 0);

    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
