// clang-format off
// RUN: %lotto %stress -r 10 -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>

#include <lotto/order.h>

int x = 0;

void *
writer()
{
    x = 1;
    lotto_order(1);
    return NULL;
}

void *
reader()
{
    lotto_order(2);
    assert(x == 1);
    return NULL;
}

int
main()
{
    pthread_t writer_thread, reader_thread;
    pthread_create(&reader_thread, 0, reader, 0);
    pthread_create(&writer_thread, 0, writer, 0);
    pthread_join(reader_thread, 0);
    pthread_join(writer_thread, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
