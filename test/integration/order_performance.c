// clang-format off
// RUN: (! %lotto %stress -r 1 -- %b 2>&1) | filecheck %s
// CHECK: assert failed {{.*}}/order_performance.c:{{[0-9]+}}: x != 0b11111
// clang-format on

#include <assert.h>
#include <pthread.h>

#include <lotto/order.h>

int x = 0;

void *
adder()
{
    lotto_order(2);
    x++;
    lotto_order(3);
    lotto_order(6);
    x++;
    lotto_order(7);
    lotto_order(10);
    x++;
    lotto_order(11);
    lotto_order(14);
    x++;
    lotto_order(15);
    lotto_order(18);
    x++;
    assert(x != 0b11111);
    return NULL;
}

void *
multiplier()
{
    x *= 2;
    lotto_order(1);
    lotto_order(4);
    x *= 2;
    lotto_order(5);
    lotto_order(8);
    x *= 2;
    lotto_order(9);
    lotto_order(12);
    x *= 2;
    lotto_order(13);
    lotto_order(16);
    x *= 2;
    lotto_order(17);
    return NULL;
}

int
main()
{
    pthread_t multiplier_thread, adder_thread;
    pthread_create(&adder_thread, 0, adder, 0);
    pthread_create(&multiplier_thread, 0, multiplier, 0);
    pthread_join(adder_thread, 0);
    pthread_join(multiplier_thread, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
