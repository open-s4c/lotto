// clang-format off
// ALLOW_RETRIES: 100
// RUN: rm -f %t.cat
// RUN: %lotto %stress -r 1 --enforce-mode CAT --record-granularity CAPTURE -- %b %t.cat
// RUN: (! %lotto %replay 3>&2 2>&1 1>&3) | %check %s
// CHECK: MISMATCH [field: cat,
// clang-format on

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

volatile int x;
static const char *marker;

void *
run_alice(void *arg)
{
    (void)arg;
    if (access(marker, F_OK) == 0) {
        volatile int y = x;
        (void)y;
        return NULL;
    }

    int fd = open(marker, O_CREAT | O_WRONLY, 0600);
    if (fd >= 0) {
        close(fd);
    }
    x = 1;
    return NULL;
}

int
main(int argc, char **argv)
{
    pthread_t alice;
    if (argc < 2) {
        return 1;
    }
    marker = argv[1];
    x = 0;
    pthread_create(&alice, 0, run_alice, 0);
    pthread_join(alice, 0);
    return 0;
}
