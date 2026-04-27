#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "qlotto_guest.h"

static int
run_command(char *const argv[])
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "qlotto-run: fork failed: %s\n", strerror(errno));
        return 127;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "qlotto-run: execvp(%s) failed: %s\n", argv[0],
                strerror(errno));
        _exit(127);
    }

    if (waitpid(pid, &status, 0) < 0) {
        fprintf(stderr, "qlotto-run: waitpid failed: %s\n", strerror(errno));
        return 127;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status)) {
        fprintf(stderr, "qlotto-run: child died from signal %d\n",
                WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }

    return 127;
}

int
main(int argc, char **argv)
{
    int rc;

    if (argc < 2) {
        fprintf(stderr, "usage: %s command [args...]\n", argv[0]);
        qlotto_exit(QLOTTO_EXIT_KIND_FAILURE);
    }

    qlotto_bias_policy(QLOTTO_BIAS_POLICY_NONE);
    qlotto_snapshot();

    rc = run_command(&argv[1]);
    if (rc == 0)
        qlotto_exit(QLOTTO_EXIT_KIND_SUCCESS);

    qlotto_exit(QLOTTO_EXIT_KIND_FAILURE);
}
