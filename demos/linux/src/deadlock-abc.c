
#include <assert.h>
#include <pthread.h>
#include <sched.h>

#include "mutex.h"

static pthread_mutex_t lock1;
static pthread_mutex_t lock2;
static pthread_mutex_t lock3;
static int x;

void *
alice(void *_)
{
    pthread_mutex_lock(&lock1);
    sched_yield();
    pthread_mutex_lock(&lock2);
    sched_yield();
    x++;
    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);
    return 0;
}

void *
bob(void *_)
{
    pthread_mutex_lock(&lock2);
    sched_yield();
    pthread_mutex_lock(&lock3);
    x++;
    pthread_mutex_unlock(&lock3);
    pthread_mutex_unlock(&lock2);
    return 0;
}

void *
eve(void *_)
{
    pthread_mutex_lock(&lock3);
    sched_yield();
    pthread_mutex_lock(&lock1);
    x++;
    pthread_mutex_unlock(&lock1);
    pthread_mutex_unlock(&lock3);
    return 0;
}
int
main()
{
    pthread_t a, b, c;
    pthread_create(&a, 0, alice, 0);
    pthread_create(&b, 0, bob, 0);
    pthread_create(&c, 0, eve, 0);
    pthread_join(a, 0);
    pthread_join(b, 0);
    pthread_join(c, 0);

    assert(x == 3);
    return 0;
}
