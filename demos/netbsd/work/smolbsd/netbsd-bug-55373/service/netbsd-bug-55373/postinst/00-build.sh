#!/bin/sh
set -eu

bindir="usr/local/bin"
libexecdir="usr/local/libexec"
incdir="usr/local/include"
srcdir="usr/src/bug-55373"

mkdir -p "${bindir}" "${libexecdir}" "${incdir}" "${srcdir}"

cp ../service/netbsd-bug-55373/qlotto_guest.h "${incdir}/qlotto_guest.h"
cp ../service/netbsd-bug-55373/qlotto_run_command.c "${srcdir}/qlotto_run_command.c"

cat > "${bindir}/bug-55373-run.sh" <<'INNER'
#!/bin/sh
set -eu

iters=${BUG_55373_ITERS:-250}
workers=${BUG_55373_WORKERS:-8}
rounds=${BUG_55373_ROUNDS:-128}
sems=${BUG_55373_SEMS:-32}

echo "bug-55373: pshared semaphore stress iters=${iters} workers=${workers} rounds=${rounds} sems=${sems}"
exec /usr/local/bin/bug-55373 	-i "${iters}" 	-w "${workers}" 	-r "${rounds}" 	-s "${sems}"
INNER
chmod 755 "${bindir}/bug-55373-run.sh"

cat > "${srcdir}/bug-55373.c" <<'INNER'
#define _NETBSD_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct shared_state {
    sem_t sems[64];
};

static void
usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [-i iterations] [-w workers] [-r rounds] [-s semaphores]\n",
        prog);
    exit(2);
}

static unsigned long
parse_ul(const char *arg, const char *name)
{
    char *end;
    unsigned long value;

    errno = 0;
    value = strtoul(arg, &end, 10);
    if (errno != 0 || *arg == '\0' || *end != '\0') {
        fprintf(stderr, "bug-55373: invalid %s: %s\n", name, arg);
        exit(2);
    }
    return value;
}

static void
worker_run(struct shared_state *state, unsigned long sems, unsigned long rounds,
    unsigned long worker)
{
    unsigned long r;
    unsigned long idx;

    for (r = 0; r < rounds; r++) {
        idx = (r + worker) % sems;
        if (sem_wait(&state->sems[idx]) != 0) {
            perror("sem_wait");
            _exit(10);
        }
        if (sem_post(&state->sems[idx]) != 0) {
            perror("sem_post");
            _exit(11);
        }
    }

    _exit(0);
}

int
main(int argc, char **argv)
{
    unsigned long iterations = 250;
    unsigned long workers = 8;
    unsigned long rounds = 128;
    unsigned long sems = 32;
    struct shared_state *state;
    size_t mapsz;
    unsigned long i;

    while ((i = (unsigned long)getopt(argc, argv, "i:w:r:s:h")) != (unsigned long)-1) {
        switch ((int)i) {
        case 'i':
            iterations = parse_ul(optarg, "iterations");
            break;
        case 'w':
            workers = parse_ul(optarg, "workers");
            break;
        case 'r':
            rounds = parse_ul(optarg, "rounds");
            break;
        case 's':
            sems = parse_ul(optarg, "semaphores");
            break;
        case 'h':
        default:
            usage(argv[0]);
        }
    }

    if (sems == 0 || sems > 64 || workers == 0) {
        fprintf(stderr, "bug-55373: unsupported parameters\n");
        return 2;
    }

    mapsz = sizeof(*state);
    state = mmap(NULL, mapsz, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    if (state == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    for (i = 0; i < iterations; i++) {
        unsigned long j;

        memset(state, 0, mapsz);

        for (j = 0; j < sems; j++) {
            if (sem_init(&state->sems[j], 1, 1) != 0) {
                perror("sem_init");
                return 1;
            }
        }

        for (j = 0; j < workers; j++) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                return 1;
            }
            if (pid == 0)
                worker_run(state, sems, rounds, j);
        }

        for (j = 0; j < workers; j++) {
            int status;
            if (wait(&status) < 0) {
                perror("wait");
                return 1;
            }
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                fprintf(stderr, "bug-55373: child failure on iteration %lu\n", i + 1);
                return 1;
            }
        }

        for (j = 0; j < sems; j++) {
            if (sem_destroy(&state->sems[j]) != 0) {
                perror("sem_destroy");
                return 1;
            }
        }

        if (((i + 1) % 25) == 0)
            printf("bug-55373: completed %lu/%lu iterations\n", i + 1, iterations);
    }

    printf("bug-55373: workload completed\n");
    return 0;
}
INNER

cat > "${libexecdir}/bug-55373-prepare.sh" <<'INNER'
#!/bin/sh
set -eu

srcdir=/usr/src/bug-55373
bindir=/usr/local/bin
incdir=/usr/local/include

if [ -x "${bindir}/qlotto-run" ] && [ -x "${bindir}/bug-55373" ]; then
	exit 0
fi

if ! command -v cc >/dev/null 2>&1; then
	echo "bug-55373: cc not found in guest"
	exit 1
fi

cc -O2  -I"${incdir}" 	-o "${bindir}/qlotto-run" "${srcdir}/qlotto_run_command.c"

cc -O2 -Wall -Wextra -I"${incdir}" 	-o "${bindir}/bug-55373" "${srcdir}/bug-55373.c" -pthread

chmod 755 "${bindir}/qlotto-run" "${bindir}/bug-55373"
INNER

chmod 755 "${libexecdir}/bug-55373-prepare.sh"
