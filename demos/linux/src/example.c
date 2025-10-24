#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/qemu.h>
#include <lotto/yield.h>
#include <vsync/queue/bounded_mpmc.h>


typedef struct {
    int producer_id;
    char payload[256];
} message;

#define NPRODUCERS 2
bounded_mpmc_t q;

int get_unique_id();

void *
producer(void *_)
{
    cpu_set_t mask;
    int cpu_id = ((((uintptr_t)(&mask)) / 1337) % 4);
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    lotto_fine_capture();

    int my_id        = get_unique_id();
    message *msg     = (message *)malloc(sizeof(message));
    msg->producer_id = my_id;

    while (bounded_mpmc_enq(&q, msg) != QUEUE_BOUNDED_OK) {}

    lotto_coarse_capture();
    return 0;
}

int nrecv = 0;
bool recv_from[NPRODUCERS];
void *
consumer(void *_)
{
    //    lotto_enter_region();
    message *msg;

    while (nrecv != NPRODUCERS) {
        if (bounded_mpmc_deq(&q, (void *)&msg) != QUEUE_BOUNDED_OK)
            continue;

        // record that received from this producer
        recv_from[msg->producer_id] = true;

        // record that another message was received
        nrecv++;

        // consume(msg)
        // ...

        free(msg);
    }
    //    lotto_leave_region();
    return 0;
}

int
main()
{
    pthread_t t[NPRODUCERS + 1];
    void *buf[1000];
    bounded_mpmc_init(&q, buf, 1000);

    // create N producers and 1 consumer
    for (int i = 0; i < NPRODUCERS; i++) {
        pthread_create(&t[i], 0, producer, 0);
    }
    pthread_create(&t[NPRODUCERS], 0, consumer, 0);

    // wait for all of them to finish
    for (int i = 0; i <= NPRODUCERS; i++) {
        pthread_join(t[i], 0);
    }

    // consumer must receive N messages
    assert(nrecv == NPRODUCERS);

    // consumer must receive a message of each producer
    for (int i = 0; i < NPRODUCERS; i++) {
        assert(recv_from[i] == true);
    }
    return 0;
}

#ifndef KERNEL_EXAMPLE
// pthread_mutex_t l;
int next_id = 0;
int
get_unique_id()
{
    // pthread_mutex_lock(&l);
    int id = next_id;
    sched_yield();
    next_id = id + 1;
    // pthread_mutex_unlock(&l);

    return id;
}
#else
int
get_unique_id()
{
    int id = 0;
    int fd = open("/proc/example", O_RDONLY);
    assert(fd >= 0);

    assert(read(fd, &id, sizeof(id)) == sizeof(id));

    close(fd);
    printf("id: %d\n", id);

    return id;
}
#endif
