#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <lotto/qemu.h>

int
test(char *cmd)
{
    return system(cmd);
}


int
main(int argc, char *argv[])
{
    if (argc == 1)
        return system("/bin/sh");

    int ret;
    unsigned long i = 0;

    do {
        printf("Iteration %lu\n", ++i);
    } while ((ret = test(argv[1])) == 0 && i < 1000);

    if (ret) {
        fprintf(stderr, "error: '%s' exited with code %d\n", argv[1], ret);
        lotto_fail();
    } else {
        lotto_halt();
    }
    return ret;
}
