// clang-format off
// UNSUPPORTED: aarch64
// RUN: %lotto %trace -- %b
// RUN: ( LD_PRELOAD=%t/libruntime.so:%t/libplotto.so:%t/libengine.so LD_LIBRARY_PATH=%t LOTTO_TEMP_DIR=%t LOTTO_LOGGER_LEVEL=debug LOTTO_RECORDER_TYPE=flat %b || true ) 2>&1 | filecheck %s
// CHECK: CAPTURE
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <vsync/queue/bounded_mpmc.h>

pthread_mutex_t l;

#define NPRODUCERS 2
bounded_mpmc_t q;
vatomic32_t next_id = VATOMIC_INIT(0);
int nrecv           = 0;
bool recv_from[NPRODUCERS];
int producer_data[NPRODUCERS];

void *producer(void *);
void *consumer(void *);

static uint32_t
get_unique_id(void)
{
    // pthread_mutex_lock(&l);
    uint32_t id = vatomic32_read(&next_id);
    sched_yield();
    vatomic32_write(&next_id, id + 1);
    // pthread_mutex_unlock(&l);

    return id;
}

void *
producer(void *_)
{
    (void)_;

    int *data = (int *)_; // malloc(sizeof(int));
    *data     = get_unique_id();
    while (1) {
        int r = bounded_mpmc_enq(&q, data);
        if (r == QUEUE_BOUNDED_OK)
            break;
    }
    return 0;
}

void *
consumer(void *_)
{
    (void)_;

    while (nrecv != NPRODUCERS) {
        void *data;
        int r = bounded_mpmc_deq(&q, &data);
        if (r != QUEUE_BOUNDED_OK)
            continue;
        int *id = (int *)data;
        assert(*id < NPRODUCERS && *id >= 0);
        recv_from[*id] = true;
        // free(id);

        nrecv++;
    }
    return 0;
}

int
main()
{
    pthread_t alice, bob, charlie;
    void *buf[1000];
    bounded_mpmc_init(&q, buf, 1000);
    vatomic32_write(&next_id, 0);
    nrecv = 0;
    for (int i = 0; i < NPRODUCERS; i++) {
        recv_from[i] = false;
    }

    pthread_create(&alice, 0, producer, producer_data);
    pthread_create(&bob, 0, producer, producer_data + 1);
    pthread_create(&charlie, 0, consumer, 0);
    pthread_join(alice, 0);
    pthread_join(bob, 0);
    pthread_join(charlie, 0);

    assert(nrecv == NPRODUCERS);
    for (int i = 0; i < NPRODUCERS; i++) {
        assert(recv_from[i] == true);
    }

    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
