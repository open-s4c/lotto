#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/sys/ensure.h>
#include <lotto/sys/memmgr_user.h>
#include <sys/mman.h>

#define FORWARD(F, ...) CONCAT(memmgr_user_, F)(__VA_ARGS__)

static long pagesize;

static void
handler(int signum, siginfo_t *si, void *vcontext)
{
    printf("Get SEGFAULT as expect\n");
    exit(0);
}

void
test_uaf()
{
    char *addr_3 = FORWARD(alloc, 10);
    ENSURE(addr_3);

    addr_3[0] = 'F';
    ENSURE('F' == addr_3[0]);
    char *addr_4 = FORWARD(realloc, (void *)addr_3, 20);

    ENSURE(addr_4[0] == 'F');

    char res = addr_3[2];
    (void)res;
    printf("Should not reach here\n");
}

void
test_alloc()
{
    void *addr_1 = FORWARD(alloc, 10);
    ENSURE(addr_1);
    FORWARD(free, addr_1);

    void *addr_2 = FORWARD(alloc, pagesize);
    ENSURE(addr_2);
}

int
main()
{
    pagesize = sysconf(_SC_PAGE_SIZE);

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));

    action.sa_flags     = SA_SIGINFO;
    action.sa_sigaction = handler;
    sigaction(SIGSEGV, &action, NULL);

    test_alloc();
    test_uaf();
}
