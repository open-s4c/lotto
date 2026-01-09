// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress -s random --stable-address-method MAP --handler-address enable --handler-cas enable --handler-stacktrace enable --handler-event enable -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -t utf-8 -f utf-8 -c | filecheck %s


// clang-format on

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

atomic_int flag = 0;

void *
f1(void *arg)
{
    flag = 1;
    return NULL;
}

void *
f2(void *arg)
{
    flag = 1;
    return NULL;
}

void *
f3(void *arg)
{
    assert(flag == 0);
    return NULL;
}

int
main()
{
    pthread_t t1, t2, t3;

    pthread_create(&t1, NULL, f1, NULL);
    pthread_create(&t2, NULL, f2, NULL);
    pthread_create(&t3, NULL, f3, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
