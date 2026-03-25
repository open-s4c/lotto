// clang-format off
// RUN: %lotto %record -- %b | grep GHOST_LOG > %b.record
// RUN: %lotto %replay | grep GHOST_LOG > %b.replay
// RUN: diff %b.record %b.replay
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>

#include <lotto/unsafe/ghost.h>

static void *
thread_(void *arg)
{
    (void)arg;

    printf("GHOST_LOG child start\n");
    lotto_ghost({
        printf("GHOST_LOG child before-exit\n");
        sched_yield();
        pthread_exit(NULL);
    });
}

int
main(void)
{
    pthread_t thread;

    setbuf(stdout, NULL);

    printf("GHOST_LOG main before-create\n");
    assert(pthread_create(&thread, NULL, thread_, NULL) == 0);
    assert(pthread_join(thread, NULL) == 0);
    printf("GHOST_LOG main joined\n");
    return 0;
}
