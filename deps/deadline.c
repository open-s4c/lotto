/* Copyright (c) 2026 Diogo Behrens
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

enum {
    /* No standard C/POSIX header defines timeout-tool exit statuses.
     * Use the GNU timeout convention explicitly: 124 means the child timed
     * out, 125 means this wrapper failed internally, and 127 follows the usual
     * shell convention for exec failure.
     */
    DEADLINE_EXIT_TIMEOUT  = 124,
    DEADLINE_EXIT_INTERNAL = 125,
    DEADLINE_EXIT_EXEC     = 127,
};

static int
parse_seconds(const char *arg, unsigned *out)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(arg, &end, 10);
    if (errno != 0 || end == arg) {
        return -1;
    }
    if (end[0] == 's' && end[1] == '\0') {
        end++;
    } else if (end[0] == 'm' && end[1] == '\0') {
        if (value > (unsigned long)UINT_MAX / 60) {
            return -1;
        }
        value *= 60;
        end++;
    }
    if (*end != '\0') {
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
    fprintf(stderr, "usage: %s [-k kill_timeout] timeout command [args...]\n",
            argv0);
    fprintf(stderr,
            "       time values may be written as X or Xs for seconds, or Xm "
            "for minutes\n");
}

static int
kill_child(pid_t child, int sig)
{
    if (kill(-child, sig) == 0) {
        return 0;
    }
    if (errno == ESRCH || kill(child, sig) == 0 || errno == ESRCH) {
        return 0;
    }
    return -1;
}

static void
cleanup_child_process_group(pid_t child)
{
    kill_child(child, SIGTERM);
    kill_child(child, SIGKILL);
}

int
main(int argc, char **argv)
{
    unsigned limit      = 0;
    unsigned kill_limit = 0;
    int argi            = 1;
    pid_t child;
    unsigned elapsed = 0;
    bool term_sent   = false;
    int status       = 0;

    if (argc < 3) {
        usage(argv[0]);
        return 2;
    }

    if (strcmp(argv[argi], "-k") == 0) {
        if (argc < 5) {
            usage(argv[0]);
            return 2;
        }
        if (parse_seconds(argv[argi + 1], &kill_limit) != 0) {
            fprintf(stderr, "%s: invalid kill timeout: %s\n", argv[0],
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
    if (kill_limit == 0) {
        if (limit == UINT_MAX) {
            fprintf(stderr, "%s: timeout too large for default kill timeout\n",
                    argv[0]);
            return 2;
        }
        kill_limit = limit + 1;
    }
    if (kill_limit < limit) {
        fprintf(stderr, "%s: kill timeout is before timeout\n", argv[0]);
        return 2;
    }

    if (argi >= argc) {
        usage(argv[0]);
        return 2;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        return DEADLINE_EXIT_INTERNAL;
    }
    if (child == 0) {
        setpgid(0, 0);
        execvp(argv[argi], &argv[argi]);
        perror(argv[argi]);
        _exit(DEADLINE_EXIT_EXEC);
    }
    setpgid(child, child);

    for (;;) {
        pid_t waited = waitpid(child, &status, WNOHANG);
        if (waited == child) {
            break;
        }
        if (waited < 0) {
            perror("waitpid");
            kill(child, SIGKILL);
            return DEADLINE_EXIT_INTERNAL;
        }

        if (!term_sent && elapsed >= limit) {
            if (kill_child(child, SIGTERM) < 0) {
                perror("kill(SIGTERM)");
                kill_child(child, SIGKILL);
                return DEADLINE_EXIT_INTERNAL;
            }
            term_sent = true;
        } else if (term_sent && elapsed >= kill_limit) {
            if (kill_child(child, SIGKILL) < 0) {
                perror("kill(SIGKILL)");
                return DEADLINE_EXIT_INTERNAL;
            }
            if (waitpid(child, &status, 0) < 0) {
                perror("waitpid");
                return DEADLINE_EXIT_INTERNAL;
            }
            break;
        }

        sleep(1);
        elapsed++;
    }

    cleanup_child_process_group(child);
    if (term_sent) {
        return DEADLINE_EXIT_TIMEOUT;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return DEADLINE_EXIT_INTERNAL;
}
