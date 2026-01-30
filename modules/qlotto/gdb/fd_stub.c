#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

#include <lotto/unsafe/_sys.h>

#define DUMMY_FILE "/dev/null"
#define FD_SPACE   32

typedef struct _state_fd_stub_s {
    int fds[FD_SPACE];
    bool fds_used[FD_SPACE];
} _state_fd_stub_t;

static _state_fd_stub_t _state = {-1};

static int32_t
_fd_stub_fd_to_idx(int fd)
{
    for (int i = 0; i < FD_SPACE; i++) {
        if (fd == _state.fds[i])
            return i;
    }
    return -1;
}

static int32_t
_fd_stub_next_free_idx(void)
{
    for (int i = 0; i < FD_SPACE; i++) {
        if (!_state.fds_used[i])
            return i;
    }
    return -1;
}

int
fd_stub_register(int tmp_fd)
{
    int32_t idx = _fd_stub_next_free_idx();
    ASSERT(idx >= 0 && idx < FD_SPACE);

    ASSERT(!_state.fds_used[idx]);

    rl_dup2(tmp_fd, _state.fds[idx]);
    rl_close(tmp_fd);

    _state.fds_used[idx] = true;

    return _state.fds[idx];
}

int
fd_stub_deregister(int fd)
{
    int32_t idx = _fd_stub_fd_to_idx(fd);
    ASSERT(idx >= 0 && idx < FD_SPACE);
    ASSERT(_state.fds_used[idx]);

    int tmp_fd = rl_open(DUMMY_FILE, O_RDONLY, 0);

    rl_dup2(tmp_fd, fd);
    rl_close(tmp_fd);

    tmp_fd = fd;

    ASSERT(tmp_fd >= 0);

    _state.fds_used[idx] = false;

    return tmp_fd;
}

void
fd_stub_init(void)
{
    for (int i = 0; i < FD_SPACE; i++) {
        _state.fds[i]      = rl_open(DUMMY_FILE, O_RDONLY, 0);
        _state.fds_used[i] = false;
        ASSERT(_state.fds[i] >= 0);
    }
}
