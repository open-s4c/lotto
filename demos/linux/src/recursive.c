
#include <assert.h>
#include <pthread.h>
#include <sched.h>

#define RECURSIVE
#include "mutex.h"

static pthread_mutex_t lock1;
static pthread_mutex_t lock2;
static int x;

void *
alice(void *_)
{
    pthread_mutex_lock(&lock1);
    pthread_mutex_lock(&lock1);
    sched_yield();
    pthread_mutex_lock(&lock2);
    x++;
    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);
    pthread_mutex_unlock(&lock1);
    return 0;
}

void *
bob(void *_)
{
    pthread_mutex_lock(&lock2);
    pthread_mutex_lock(&lock1);
    sched_yield();
    x++;
    pthread_mutex_unlock(&lock1);
    pthread_mutex_unlock(&lock2);
    return 0;
}

int
main()
{
    pthread_t a, b;
    pthread_create(&a, 0, alice, 0);
    pthread_create(&b, 0, bob, 0);
    pthread_join(a, 0);
    pthread_join(b, 0);

    assert(x == 2);
    return 0;
}
