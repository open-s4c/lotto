// clang-format off
// UNSUPPORTED: aarch64, clang
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/safe_stack.log -- %b 2>&1) | %check %s
// CHECK: [{{[0-9]+}}] assert failed {{.*}}/safe_stack.c:{{[0-9]+}}: array[elem].value == idx
// CHECK: [lotto] Trace reconstructed successfully!
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdint.h>

#include "safe_stack.h"

void *
worker(void *arg)
{
    intptr_t idx = ((intptr_t)arg);
    int elem;

    if ((elem = pop()) >= 0) {
        lotto_log_data(22, elem);
        array[elem].value = idx;
        lotto_log(23);

        lotto_log(24);
        assert(array[elem].value == idx);
        lotto_log(25);

        push(elem);

        if ((elem = pop()) >= 0) {
            lotto_log_data(26, elem);
            array[elem].value = idx;
            lotto_log(27);

            lotto_log(28);
            assert(array[elem].value == idx);
            lotto_log(29);

            push(elem);
        }
    }

    return NULL;
}

int
main()
{
    init();

    pthread_t t0, t1, t2;
    pthread_create(&t0, NULL, worker, (void *)0);
    pthread_create(&t1, NULL, worker, (void *)1);
    pthread_create(&t2, NULL, worker, (void *)2);
    pthread_join(t0, 0);
    pthread_join(t1, 0);
    pthread_join(t2, 0);

    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
