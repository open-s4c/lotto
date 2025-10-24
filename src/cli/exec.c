/*
 */

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE /* See feature_test_macros(7) */
#endif
#include <crep.h>
#include <errno.h>
#include <spawn.h>

#include <lotto/cli/exec.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/sys/poll.h>
#include <lotto/sys/signal.h>
#include <lotto/sys/spawn.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <lotto/sys/unistd.h>
#include <lotto/sys/wait.h>
#include <lotto/util/casts.h>

static pid_t _pid;
static int p_out[2];
static int p_err[2];


static void
_handle_sigint(int sig, siginfo_t *si, void *arg)
{
    if (_pid) {
        sys_fprintf(stderr, "[lotto] SIGINT\n");
        sys_kill(0, SIGINT);
        int wstatus;
        sys_waitpid(0, &wstatus, 0);
        sys_exit(130);
    }
    sys_exit(1);
}

static void
_handle_sigterm(int sig, siginfo_t *si, void *arg)
{
    if (_pid) {
        sys_fprintf(stderr, "[lotto] SIGTERM\n");
        sys_kill(0, SIGTERM);
        int wstatus;
        sys_waitpid(0, &wstatus, 0);
        sys_exit(130);
    }
    sys_exit(1);
}

static void
_handle_sigkill(int sig, siginfo_t *si, void *arg)
{
    if (_pid) {
        sys_fprintf(stderr, "[lotto] SIGKILL\n");
        sys_kill(0, SIGKILL);
        int wstatus;
        sys_waitpid(0, &wstatus, 0);
        sys_exit(130);
    }
    sys_exit(1);
}

WEAK void
crep_truncate(clk_t clk)
{
    (void)clk;
}

#define BUFFER_SIZE 1024

void
read_pipes()
{
    char buffer[BUFFER_SIZE];
    nfds_t nfds = 2;
    ssize_t bytes_read;
    struct timespec timeout_ts;
    struct pollfd pfds[2];
    int ret_ppoll = 0;

    sys_setvbuf(stdout, NULL, _IONBF, 0);
    sys_setvbuf(stderr, NULL, _IONBF, 0);

    pfds[0].fd     = p_out[0];
    pfds[0].events = POLLIN;

    pfds[1].fd     = p_err[0];
    pfds[1].events = POLLIN;

    timeout_ts.tv_sec  = 0;
    timeout_ts.tv_nsec = 10000;

    // revents can only be POLLIN, POLLERR, POLLHUP, POLLNVAL
    while (nfds > 0 && ret_ppoll >= 0) {
        ret_ppoll      = sys_ppoll(pfds, nfds, &timeout_ts, NULL);
        bool data_read = false;
        // POLLINs
        for (uint64_t i = 0; i < nfds; i++) {
            if (!(pfds[i].revents & POLLIN)) {
                continue;
            }
            bytes_read = sys_read(pfds[i].fd, buffer, BUFFER_SIZE - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                sys_fprintf(pfds[i].fd == p_out[0] ? stdout : stderr, "%s",
                            buffer);
            }
            data_read = true;
        }
        if (data_read)
            continue;

        for (int64_t i = CAST_TYPE(int64_t, nfds) - 1; i >= 0; i--) {
            if (!(pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))) {
                continue;
            }
            pfds[i].fd = pfds[--nfds].fd;
        }
    }

    if (ret_ppoll == -1) {
        switch (errno) {
            case EFAULT:
            case EINTR:
            case EINVAL:
            case ENOMEM:
                ASSERT(0 && "Expecting no error on ppoll");
                break;
            default:
                ASSERT(0 && "unchecked ppoll error");
                break;
        }
    }
    return;
}

/* returns err */
static int
wait_child(pid_t pid)
{
    int wstatus, retval = 0;
    read_pipes();
    while (1) {
        pid_t rpid = sys_waitpid(pid, &wstatus, WNOHANG);
        if (rpid == 0) // not dead
            continue;
        if (errno == ECHILD) {
            sys_fprintf(stdout, "Terminated unexpectedly\n");
            retval = 1;
            break;
        }
        if (rpid < 0) {
            sys_fprintf(stdout, "Unexpected return :%d\n", rpid);
        }
        if (WIFEXITED(wstatus)) {
            int status = WEXITSTATUS(wstatus);
            // if (status != 0)
            //     sys_fprintf(stdout,"exited with code: %d\n",
            //     status);
            retval = status;
            break;
        }
        if (WIFSIGNALED(wstatus)) {
            sys_fprintf(stdout, "Signal received \n");
            // WTERMSIG(wstatus)
            retval = 1;
            break;
        }
        if (WIFSTOPPED(wstatus)) {
            retval = 1;
            break;
        }
    }
    sys_close(p_out[0]);
    sys_close(p_err[0]);
    return retval;
}

DECLARE_FLAG_BEFORE_RUN;
DECLARE_FLAG_AFTER_RUN;
DECLARE_FLAG_REPLAY_GOAL;

int
execute(const args_t *args, const flags_t *flags, bool config)
{
    const char *cmd = flags_get_sval(flags, FLAG_BEFORE_RUN);
    if (cmd && cmd[0]) {
        sys_setenv("LOTTO_DISABLE", "true", true);
        int ret = system(cmd);
        sys_unsetenv("LOTTO_DISABLE");
        ASSERT(ret != -1 && "could not run the before run action");
    }
    const char *replay_ptr = sys_getenv("LOTTO_REPLAY");
    char *replay_copy;
    if (replay_ptr) {
        size_t replay_len = sys_strlen(replay_ptr);
        replay_copy       = sys_malloc(replay_len + 1);
        sys_strcpy(replay_copy, replay_ptr);
    } else {
        replay_copy = NULL;
    }
    struct flag_val fgoal = flags_get(flags, FLAG_REPLAY_GOAL);
    cli_trace_init(sys_getenv("LOTTO_RECORD"), args, sys_getenv("LOTTO_REPLAY"),
                   fgoal, config, flags);
    if (!fgoal.is_default) {
        crep_truncate(as_uval(fgoal._val));
    }

    struct sigaction int_action, int_old;
    sys_memset(&int_action, 0, sizeof(struct sigaction));
    int_action.sa_flags     = SA_SIGINFO;
    int_action.sa_sigaction = _handle_sigint;
    sys_sigaction(SIGINT, &int_action, &int_old);

    struct sigaction term_action, term_old;
    sys_memset(&term_action, 0, sizeof(struct sigaction));
    term_action.sa_flags     = SA_SIGINFO;
    term_action.sa_sigaction = _handle_sigterm;
    sys_sigaction(SIGTERM, &term_action, &term_old);

    struct sigaction kill_action, kill_old;
    sys_memset(&kill_action, 0, sizeof(struct sigaction));
    kill_action.sa_flags     = SA_SIGINFO;
    kill_action.sa_sigaction = _handle_sigkill;
    sys_sigaction(SIGKILL, &kill_action, &kill_old);

    int err = 0;

    // Set of signals to force to default signal handling in the new process:
    sigset_t sigdefset;
    sys_sigemptyset(&sigdefset);
    sys_sigaddset(&sigdefset, SIGINT);
    posix_spawnattr_t attr;
    sys_posix_spawnattr_init(&attr);
    sys_posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGDEF);
    sys_posix_spawnattr_setsigdefault(&attr, &sigdefset);

    posix_spawn_file_actions_t action;
    if (sys_pipe(p_out) || sys_pipe(p_err))
        sys_fprintf(stderr, "pipe2 returned an error.\n");

    sys_posix_spawn_file_actions_init(&action);

    sys_posix_spawn_file_actions_addclose(&action, p_out[0]);
    sys_posix_spawn_file_actions_addclose(&action, p_err[0]);

    sys_posix_spawn_file_actions_adddup2(&action, p_out[1], STDOUT_FILENO);
    sys_posix_spawn_file_actions_adddup2(&action, p_err[1], STDERR_FILENO);

    sys_posix_spawn_file_actions_addclose(&action, p_out[1]);
    sys_posix_spawn_file_actions_addclose(&action, p_err[1]);

    err =
        posix_spawnp(&_pid, args->argv[0], &action, &attr, args->argv, environ);
    if (err != 0) {
        sys_fprintf(stderr, "error creating child: %s\n", strerror(err));
    } else {
        // closes child side of pipes
        sys_close(p_out[1]);
        sys_close(p_err[1]);
        err = wait_child(_pid);
    }
    sys_posix_spawn_file_actions_destroy(&action);

    sys_sigaction(SIGINT, &int_old, NULL);
    sys_sigaction(SIGTERM, &term_old, NULL);
    sys_sigaction(SIGKILL, &kill_old, NULL);

    if (replay_ptr) {
        sys_setenv("LOTTO_REPLAY", replay_copy, true);
        sys_free(replay_copy);
    } else {
        sys_unsetenv("LOTTO_REPLAY");
    }

    cmd = flags_get_sval(flags, FLAG_AFTER_RUN);
    if (cmd && cmd[0]) {
        sys_setenv("LOTTO_DISABLE", "true", true);
        int ret = sys_system(cmd);
        sys_unsetenv("LOTTO_DISABLE");
        ASSERT(ret != -1 && "could not run the after run action");
    }

    return err;
}
