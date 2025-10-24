// clang-format off
// UNSUPPORTED: aarch64
// XFAIL: *
// RUN: exit 1
// RUN: %cleanup
// RUN: %lotto %record %b > %T/fd_test_record.log
// RUN: %lotto %replay > %T/fd_test_replay.log
// RUN: diff %T/fd_test_record.log %T/fd_test_replay.log
// clang-format on
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

void *
run()
{
    const char *fn = "file.txt";
    {
        int fd     = open(fn, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        int fd_dup = dup(fd);

        assert(fd >= 0 && fd_dup >= 0);

        ssize_t fw  = write(fd, "hello\n", 6);
        ssize_t fw2 = write(fd_dup, "hi\n", 3);

        printf("%d %ld %d %ld\n", fd, fw, fd_dup, fw2);
        close(fd);
        close(fd_dup);
    }
    {
        int fd = open(fn, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);

        char *content = (char *)calloc(100, sizeof(char));
        ssize_t fr    = read(fd, content, 100);

        printf("%d %ld\n", fd, fr);
        close(fd);
    }
    return NULL;
}

int
main()
{
    pthread_t t;
    pthread_create(&t, NULL, run, NULL);
    pthread_join(t, NULL);
    exit(0);
}
