// ALLOW_RETRIES: 10
// RUN: (! %lotto %stress -- %x) | %check
// CHECK: Deadlock detected!

#include <pthread.h>
#include <sched.h>

static pthread_mutex_t g_mutex1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_mutex2 = PTHREAD_MUTEX_INITIALIZER;

static void *
thread_a(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&g_mutex1);
    sched_yield();
    pthread_mutex_lock(&g_mutex2);
    pthread_mutex_unlock(&g_mutex2);
    pthread_mutex_unlock(&g_mutex1);
    return NULL;
}

static void *
thread_b(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&g_mutex2);
    sched_yield();
    pthread_mutex_lock(&g_mutex1);
    pthread_mutex_unlock(&g_mutex1);
    pthread_mutex_unlock(&g_mutex2);
    return NULL;
}

int
main(void)
{
    pthread_t a;
    pthread_t b;
    pthread_create(&a, NULL, thread_a, NULL);
    pthread_create(&b, NULL, thread_b, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    return 0;
}
