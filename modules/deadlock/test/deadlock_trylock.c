// RUN: %lotto %stress -r 20 -e deadlock -- %b
#include <assert.h>
#include <errno.h>
#include <pthread.h>

static pthread_mutex_t held = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t available = PTHREAD_MUTEX_INITIALIZER;

static void *trylocks(void *unused)
{
    (void)unused;
    /* The failed attempt must not become a held/lost resource on thread exit. */
    assert(pthread_mutex_trylock(&held) == EBUSY);
    /* A successful attempt must still be tracked, including its release. */
    assert(pthread_mutex_trylock(&available) == 0);
    assert(pthread_mutex_unlock(&available) == 0);
    return NULL;
}

int main(void)
{
    pthread_t thread;
    assert(pthread_mutex_lock(&held) == 0);
    assert(pthread_create(&thread, NULL, trylocks, NULL) == 0);
    assert(pthread_join(thread, NULL) == 0);
    assert(pthread_mutex_unlock(&held) == 0);
    assert(pthread_mutex_destroy(&available) == 0);
    assert(pthread_mutex_destroy(&held) == 0);
    return 0;
}
