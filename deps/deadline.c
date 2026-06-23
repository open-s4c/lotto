#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int
parse_seconds(const char *arg, unsigned *out)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(arg, &end, 10);
    if (errno != 0 || end == arg || *end != '\0') {
        return -1;
    }
    if (value > (unsigned long)UINT_MAX) {
        return -1;
    }
    *out = (unsigned)value;
    return 0;
}

static void
usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [-k grace_seconds] seconds command [args...]\n",
            argv0);
}

int
main(int argc, char **argv)
{
    unsigned limit = 0;
    unsigned grace = 0;
    int argi       = 1;
    pid_t child;
    unsigned elapsed = 0;
    bool term_sent = false;
    int status = 0;

    if (argc < 3) {
        usage(argv[0]);
        return 2;
    }

    if (strcmp(argv[argi], "-k") == 0) {
        if (argc < 5) {
            usage(argv[0]);
            return 2;
        }
        if (parse_seconds(argv[argi + 1], &grace) != 0) {
            fprintf(stderr, "%s: invalid grace period: %s\n", argv[0],
                    argv[argi + 1]);
            return 2;
        }
        argi += 2;
    }

    if (parse_seconds(argv[argi], &limit) != 0) {
        fprintf(stderr, "%s: invalid timeout: %s\n", argv[0], argv[argi]);
        return 2;
    }
    argi++;

    if (argi >= argc) {
        usage(argv[0]);
        return 2;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        return 125;
    }
    if (child == 0) {
        execvp(argv[argi], &argv[argi]);
        perror(argv[argi]);
        _exit(127);
    }

    for (;;) {
        pid_t waited = waitpid(child, &status, WNOHANG);
        if (waited == child) {
            break;
        }
        if (waited < 0) {
            perror("waitpid");
            kill(child, SIGKILL);
            return 125;
        }

        if (!term_sent && elapsed >= limit) {
            if (kill(child, SIGTERM) < 0 && errno != ESRCH) {
                perror("kill(SIGTERM)");
                kill(child, SIGKILL);
                return 125;
            }
            term_sent = true;
        } else if (term_sent && elapsed >= limit + grace) {
            if (kill(child, SIGKILL) < 0 && errno != ESRCH) {
                perror("kill(SIGKILL)");
                return 125;
            }
        }

        sleep(1);
        elapsed++;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 125;
}
