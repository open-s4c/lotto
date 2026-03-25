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
    sched_yield();
    printf("GHOST_LOG child end\n");
    return NULL;
}

int
main(void)
{
    pthread_t thread;

    setbuf(stdout, NULL);

    lotto_ghost({
        printf("GHOST_LOG main before-create\n");
        sched_yield();
        assert(pthread_create(&thread, NULL, thread_, NULL) == 0);
        printf("GHOST_LOG main after-create\n");
        sched_yield();
    });

    assert(pthread_join(thread, NULL) == 0);
    printf("GHOST_LOG main joined\n");
    return 0;
}
