// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress -s random --stable-address-method MAP --handler-address enable --handler-cas enable --handler-stacktrace enable --handler-event enable -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -f utf-8 -t utf-8 -c > %s.out
// RUN: ( %check %s --check-prefix=CAS_FIRST < %s.out ) || ( %check %s --check-prefix=CAS_SECOND < %s.out )

// CAS_FIRST: ------ . ------ . ------ . ------ . ------
// CAS_FIRST-NEXT: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// CAS_FIRST: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// CAS_FIRST: ------ . ------ . ------ . ------ . ------
// CAS_FIRST-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// CAS_FIRST: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// CAS_SECOND: ------ . ------ . ------ . ------ . ------
// CAS_SECOND-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// CAS_SECOND: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// CAS_SECOND: ------ . ------ . ------ . ------ . ------
// CAS_SECOND-NEXT: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// CAS_SECOND: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

atomic_int x = 0;
atomic_int y = 0;

/* Two possible outcomes:
 * 1. A->C->B  (B->D) is implied
 * 2. C->A->D  (D->B) is implied
 */

void *
t1()
{
    atomic_fetch_add(&x, 1); // A
    atomic_fetch_sub(&x, 1); // B
    return NULL;
}

void *
t2()
{
    int old = atomic_load(&x); // C
    int new = 100;
    if (!atomic_compare_exchange_strong(&x, &old, new)) { // D
        assert(false);
    }
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
