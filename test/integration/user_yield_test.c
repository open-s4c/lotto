// RUN: (! %lotto %stress -- %b 2>&1) | %check
// CHECK: assert failed: val == 1

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>

static atomic_int g_ctrl = 0;
static atomic_int g_value = 0;

static void *
run_producer(void *arg)
{
    (void)arg;
    atomic_store_explicit(&g_ctrl, 1, memory_order_release);
    sched_yield();
    atomic_store_explicit(&g_value, 1, memory_order_release);
    return NULL;
}

static void *
run_consumer(void *arg)
{
    (void)arg;
    while (atomic_load_explicit(&g_ctrl, memory_order_acquire) == 0) {
    }
    return (void *)(uintptr_t)atomic_load_explicit(&g_value, memory_order_acquire);
}

int
main(void)
{
    pthread_t producer;
    pthread_t consumer;
    pthread_create(&producer, NULL, run_producer, NULL);
    pthread_create(&consumer, NULL, run_consumer, NULL);

    pthread_join(producer, NULL);
    void *result;
    pthread_join(consumer, &result);

    return (result == (void *)1) ? 0 : 1;
}
