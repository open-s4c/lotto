// REQUIRES: module-blocking
// clang-format off
// RUN: (%lotto --load-runtime %B/pthread_create-random-sigabrt-probe.so %record -- %b; echo EXIT:$?) 2>&1 | grep -E 'THREAD|HANDLER_RANDOM:|EXIT:' > %b.record
// RUN: (%lotto --load-runtime %B/pthread_create-random-sigabrt-probe.so %replay; echo EXIT:$?) 2>&1 | grep -E 'THREAD|HANDLER_RANDOM:|EXIT:' > %b.replay
// RUN: diff %b.record %b.replay
// clang-format on

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>

#include <dice/log.h>

static void *
thread(void *arg)
{
    log_printf("THREAD %p\n", arg);
    return NULL;
}

int
main(void)
{
    pthread_t thread_id;
    int err = pthread_create(&thread_id, NULL, thread, (void *)(uint64_t)1);

    assert(err == 0);
    err = pthread_join(thread_id, NULL);
    assert(err == 0);
    return 0;
}
