// clang-format off
// RUN: (! %lotto %stress -- %b 2>&1) | filecheck %s
// CHECK: assert failed {{.*}}/drop.c:{{[0-9]+}}: x != 6
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#include <lotto/drop.h>

atomic_bool locked = false;
atomic_int x       = 0;

void
acquire()
{
    bool already_locked = atomic_exchange(&locked, true);
    if (already_locked)
        lotto_drop();
}

void
release()
{
    assert(atomic_exchange(&locked, 0));
}

void *
f1(void *arg)
{
    acquire();
    x += 1;
    release();
    return NULL;
}

void *
f2(void *arg)
{
    acquire();
    x += 2;
    release();
    return NULL;
}

void *
f3(void *arg)
{
    acquire();
    x += 3;
    release();
    return NULL;
}

void *
f4(void *arg)
{
    acquire();
    assert(x != 6);
    release();
    return NULL;
}

int
main()
{
    pthread_t t1, t2, t3, t4;
    pthread_create(&t3, NULL, f3, NULL);
    pthread_create(&t1, NULL, f1, NULL);
    pthread_create(&t2, NULL, f2, NULL);
    pthread_create(&t4, NULL, f4, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
