#include <dlfcn.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <lotto/sys/abort.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>

NORETURN void
sys_abort(void)
{
#ifndef LOTTO_TEST
    abort();
#else
    kill(0, SIGABRT);
#endif
}

NORETURN void
sys_assert_fail(const char *a, const char *file, unsigned int line,
                const char *func)
{
    log_errorf("assert failed %s(): %s:%u: %s\n", func, file,
               (unsigned int)line, a);
    fflush(stderr);
    sys_abort();
}
