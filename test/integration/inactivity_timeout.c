// clang-format off
// UNSUPPORTED: aarch64
// RUN: (! timeout 3s %lotto %stress --inactivity-timeout 1 -r 1 -- %b 2>&1) | %check %s
// CHECK: Task [2] has no capture point received after 1 seconds
// clang-format on


#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void *
thread_long_enough()
{
    while (1)
        ;
    return NULL;
}

int
main()
{
    pthread_t t1;
    pthread_create(&t1, NULL, thread_long_enough, NULL);
    pthread_join(t1, NULL);

    return 0;
}
