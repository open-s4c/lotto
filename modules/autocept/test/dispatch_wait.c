#include <dispatch/dispatch.h>
#include <stdio.h>

int
main(void)
{
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    if (sem == NULL) {
        fprintf(stderr, "dispatch_semaphore_create failed\n");
        return 1;
    }

    long ret = dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW);
    if (ret == 0) {
        fprintf(stderr, "dispatch_semaphore_wait unexpectedly acquired\n");
        return 1;
    }

    printf("dispatch_semaphore_wait returned %ld\n", ret);
    return 0;
}
