#include <errno.h>
#include <spawn.h>

#include <lotto/driver/exec.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/trace_prepare.h>
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
static exec_command_prefix_f *_exec_command_prefix;

#define DICE_DSO_ENV "DICE_DSO"
#define LOTTO_DSO    "liblotto-runtime.so"

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

#define BUFFER_SIZE 1024

typedef struct {
    int wstatus;
    bool have_status;
    bool child_missing;
} child_wait_state_t;

static bool
pipe_should_close_(short revents)
{
    return revents & (POLLERR | POLLHUP | POLLNVAL);
}

static void
record_child_exit_(pid_t pid, child_wait_state_t *state)
{
    if (state->have_status || state->child_missing) {
        return;
    }

    pid_t rpid = sys_waitpid(pid, &state->wstatus, WNOHANG);
    if (rpid == pid) {
        state->have_status = true;
        return;
    }
    if (rpid < 0 && errno == ECHILD) {
        state->child_missing = true;
    }
}

static bool
consume_pipe_data_(int fd, short revents, bool *data_read)
{
    char buffer[BUFFER_SIZE];
    bool drain_to_eof = revents & POLLHUP;
    int out_fd        = fd == p_out[0] ? STDOUT_FILENO : STDERR_FILENO;

    if (!(revents & (POLLIN | POLLHUP))) {
        return false;
    }

    while (1) {
        ssize_t bytes_read = sys_read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            sys_dprintf(out_fd, "%s", buffer);
            *data_read = true;
            if (!drain_to_eof) {
                return false;
            }
            continue;
        }
        if (bytes_read == 0) {
            return true;
        }
        switch (errno) {
            case EINTR:
                continue;
            case EAGAIN:
#if EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
                return false;
            default:
                ASSERT(0 && "unchecked read error");
                return true;
        }
    }
}

static void
read_pipes(pid_t pid, child_wait_state_t *state)
{
    nfds_t nfds             = 2;
    struct timespec timeout = {.tv_sec = 0, .tv_nsec = 10000};
    struct pollfd pfds[2]   = {{.fd = p_out[0], .events = POLLIN},
                               {.fd = p_err[0], .events = POLLIN}};

    while (nfds > 0) {
        int ret_ppoll = sys_ppoll(pfds, nfds, &timeout, NULL);
        if (ret_ppoll == -1) {
            if (errno == EINTR) {
                continue;
            }
            switch (errno) {
                case EFAULT:
                case EINVAL:
                case ENOMEM:
                    ASSERT(0 && "Expecting no error on ppoll");
                    break;
                default:
                    ASSERT(0 && "unchecked ppoll error");
                    break;
            }
        }

        record_child_exit_(pid, state);

        bool data_read     = false;
        bool close_pipe[2] = {false, false};
        for (uint64_t i = 0; i < nfds; i++) {
            short revents = pfds[i].revents;
            if (!(revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL))) {
                continue;
            }

            if (revents & (POLLIN | POLLHUP)) {
                close_pipe[i] =
                    consume_pipe_data_(pfds[i].fd, revents, &data_read);
                continue;
            }

            if (pipe_should_close_(revents)) {
                close_pipe[i] = true;
            }
        }

        for (int64_t i = CAST_TYPE(int64_t, nfds) - 1; i >= 0; i--) {
            if (!close_pipe[i]) {
                continue;
            }
            pfds[i] = pfds[--nfds];
        }

        if ((state->have_status || state->child_missing) && !data_read &&
            ret_ppoll == 0) {
            break;
        }
    }
}

static int
wait_status_to_retval_(int wstatus)
{
    if (WIFEXITED(wstatus)) {
        return WEXITSTATUS(wstatus);
    }
    if (WIFSIGNALED(wstatus)) {
        sys_fprintf(stdout, "Signal %s received\n",
                    strsignal(WTERMSIG(wstatus)));
        return -WTERMSIG(wstatus);
    }
    if (WIFSTOPPED(wstatus)) {
        return 1;
    }
    return 1;
}

/* returns err */
static int
wait_child(pid_t pid)
{
    child_wait_state_t state = {0};
    int retval               = 0;

    read_pipes(pid, &state);

    while (!state.have_status && !state.child_missing) {
        pid_t rpid = sys_waitpid(pid, &state.wstatus, 0);
        if (rpid == pid) {
            state.have_status = true;
            break;
        }
        if (rpid < 0 && errno == EINTR) {
            continue;
        }
        if (rpid < 0 && errno == ECHILD) {
            state.child_missing = true;
            break;
        }
        if (rpid < 0) {
            sys_fprintf(stdout, "Unexpected return :%d\n", rpid);
            retval = 1;
            break;
        }
    }

    if (retval == 0) {
        if (state.have_status) {
            retval = wait_status_to_retval_(state.wstatus);
        } else if (state.child_missing) {
            sys_fprintf(stdout, "Terminated unexpectedly\n");
            retval = 1;
        }
    }

    sys_close(p_out[0]);
    sys_close(p_err[0]);
    return retval;
}

int
execute(const args_t *args, const flags_t *flags, bool config)
{
    args_t prefixed_args = *args;
    char **dynamic_argv  = NULL;
    if (_exec_command_prefix != NULL) {
        char **original_argv = prefixed_args.argv;
        _exec_command_prefix(&prefixed_args, flags);
        if (prefixed_args.argv != original_argv) {
            dynamic_argv = prefixed_args.argv;
        }
    }

    const char *old_dice_dso = sys_getenv(DICE_DSO_ENV);
    char *old_dice_dso_copy  = NULL;
    if (old_dice_dso) {
        size_t len        = sys_strlen(old_dice_dso);
        old_dice_dso_copy = sys_malloc(len + 1);
        sys_strcpy(old_dice_dso_copy, old_dice_dso);
    }
    sys_setenv(DICE_DSO_ENV, LOTTO_DSO, true);

    const char *cmd = flags_get_sval(flags, flag_before_run());
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
    struct flag_val fgoal = flags_get(flags, flag_replay_goal());
    cli_trace_init(sys_getenv("LOTTO_RECORD"), &prefixed_args,
                   sys_getenv("LOTTO_REPLAY"), fgoal, config, flags);

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

    int err = 0;

    // Set of signals to force to default signal handling in the new process:
    sigset_t sigdefset;
    sys_sigemptyset(&sigdefset);
    sys_sigaddset(&sigdefset, SIGINT);
    sys_sigaddset(&sigdefset, SIGTERM);
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

    err = posix_spawnp(&_pid, prefixed_args.argv[0], &action, &attr,
                       prefixed_args.argv, environ);
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

    if (old_dice_dso_copy) {
        sys_setenv(DICE_DSO_ENV, old_dice_dso_copy, true);
        sys_free(old_dice_dso_copy);
    } else {
        sys_unsetenv(DICE_DSO_ENV);
    }

    if (replay_ptr) {
        sys_setenv("LOTTO_REPLAY", replay_copy, true);
        sys_free(replay_copy);
    } else {
        sys_unsetenv("LOTTO_REPLAY");
    }

    cmd = flags_get_sval(flags, flag_after_run());
    if (cmd && cmd[0]) {
        sys_setenv("LOTTO_DISABLE", "true", true);
        int ret = sys_system(cmd);
        sys_unsetenv("LOTTO_DISABLE");
        ASSERT(ret != -1 && "could not run the after run action");
    }

    if (dynamic_argv != NULL) {
        sys_free(dynamic_argv);
    }

    return err;
}

void
execute_set_command_prefix(exec_command_prefix_f *prefix)
{
    _exec_command_prefix = prefix;
}
