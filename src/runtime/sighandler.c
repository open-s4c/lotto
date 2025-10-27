/*
 */
#ifndef DISABLE_EXECINFO
    #include <execinfo.h>
#endif
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/record.h>
#include <lotto/base/task_id.h>
#include <lotto/runtime/intercept.h>
#include <lotto/runtime/runtime.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/string.h>
#include <lotto/sys/unistd.h>
#include <lotto/util/casts.h>
#include <vsync/atomic.h>
#include <vsync/atomic/dispatch.h>

#define LOTTO_HANDLE_ASSERT

#if defined(__linux__)
    #define line_int unsigned int
#else
    #define line_int int
#endif


REAL_DECL(void, __assert_fail, const char *assertion, const char *file,
          line_int line, const char *function);

#define BACKTRACE_DEPTH 50
static void
_backtrace_print()
{
#if !defined(QLOTTO_ENABLED)
    #ifndef DISABLE_EXECINFO
    void *array[BACKTRACE_DEPTH];
    int size;

    // get void*'s for all entries on the stack
    size = backtrace(array, BACKTRACE_DEPTH);

    // print out all the frames to stderr
    fflush(logger_fp());
    backtrace_symbols_fd(array, size, fileno(logger_fp()));
    #endif
#endif
}

#define SIGHANLDER_FD 2

static void
_write_uint64(int fd, uint64_t n)
{
    do {
        char digit = CAST_TYPE(char, '0' + n % 10);
        if (write(fd, &digit, 1) < 0) {
            return;
        }
        n /= 10;
    } while (n > 0);
}

static void
_handle(reason_t user_reason, reason_t runtime_reason)
{
    mediator_disable_registration();
    mediator_t *m   = get_mediator(false);
    context_t ctx   = {.cat = CAT_NONE};
    reason_t reason = m->registration_status != MEDIATOR_REGISTRATION_NONE &&
                              mediator_in_capture(m) ?
                          runtime_reason :
                          user_reason;
    sys_write(SIGHANLDER_FD, "[", 1);
    _write_uint64(SIGHANLDER_FD, ctx.id);
    sys_write(SIGHANLDER_FD, "]", 1);
    if (m->registration_status == MEDIATOR_REGISTRATION_NONE) {
        sys_write(SIGHANLDER_FD, "(estimated)", sys_strlen("(estimated)"));
    }
    sys_write(SIGHANLDER_FD, " ", 1);
    const char *str_reason = reason_str(reason);
    sys_write(SIGHANLDER_FD, str_reason, sys_strlen(str_reason));
    sys_write(SIGHANLDER_FD, "\n", 1);

    _backtrace_print();

    lotto_exit(&ctx, reason);
}

static void
_handle_sigint(int sig, siginfo_t *si, void *arg)
{
    _handle(REASON_SIGINT, REASON_RUNTIME_SIGINT);
}

static void
_handle_segfault(int sig, siginfo_t *si, void *arg)
{
    _handle(REASON_SEGFAULT, REASON_RUNTIME_SEGFAULT);
}

static void
_handle_sigabrt(int sig, siginfo_t *si, void *arg)
{
    _handle(REASON_SIGABRT, REASON_RUNTIME_SIGABRT);
}

static void
_handle_sigterm(int sig, siginfo_t *si, void *arg)
{
    _handle(REASON_SIGTERM, REASON_RUNTIME_SIGTERM);
}

static void
_handle_sigkill(int sig, siginfo_t *si, void *arg)
{
    _handle(REASON_SIGKILL, REASON_RUNTIME_SIGKILL);
}

static void
_set_handler(int sig, void (*handle)(int, siginfo_t *, void *))
{
    struct sigaction action;
    sys_memset(&action, 0, sizeof(struct sigaction));
    action.sa_flags     = SA_SIGINFO;
    action.sa_sigaction = handle;
    sigaction(sig, &action, NULL);
}

void
sighandler_init()
{
    _set_handler(SIGSEGV, _handle_segfault);
    _set_handler(SIGINT, _handle_sigint);
    _set_handler(SIGABRT, _handle_sigabrt);
    _set_handler(SIGTERM, _handle_sigterm);
    _set_handler(SIGKILL, _handle_sigkill);
    // block SIGTTOU and SIGTTIN
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGTTIN);
    sigaddset(&signal_set, SIGTTOU);
    sigprocmask(SIG_BLOCK, &signal_set, NULL);
}

#ifdef LOTTO_HANDLE_ASSERT
static vatomic32_t _handling;
NORETURN void
__assert_fail(const char *assertion, const char *file, line_int line,
              const char *function)
{
    if (vatomic_xchg(&_handling, 1))
        REAL(__assert_fail, assertion, file, line, function);

    long unsigned int tid = get_task_id();
    logger_println("[%lu] assert failed %s(): %s:%u: %s\n", tid, function, file,
                (unsigned int)line, assertion);

    _backtrace_print();

    context_t ctx = {
        .cat = CAT_NONE,
    };
    lotto_exit(&ctx, REASON_ASSERT_FAIL);
}

#endif
