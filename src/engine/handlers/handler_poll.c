/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK

#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/handlers/poll.h>
#include <lotto/engine/handlers/timeout.h>
#include <lotto/sys/poll.h>

/*******************************************************************************
 * state
 ******************************************************************************/
typedef struct poll {
    tiditem_t ti;
    poll_args_t *args;
} poll_t;

typedef struct fd {
    tiditem_t ti;
    short events;
    tidset_t waiters;
    uint64_t counts[sizeof(short) * 8]; // a counter per events bit
} fd_t;

static struct handler_poll {
    tidmap_t polls;
    tidmap_t fds;
    bool should_cleanup;
} _state;

REGISTER_STATE(EPHEMERAL, _state, {
    tidmap_init(&_state.polls, MARSHABLE_STATIC(sizeof(poll_t)));
    tidmap_init(&_state.fds,
                MARSHABLE_STATIC(sizeof(fd_t))); // no marshal support
})
static void _cleanup_poll(const poll_t *poll);
static void _cleanup_fds();
/* Unblock task when timeout is triggered by the timeout handler */
PS_SUBSCRIBE_INTERFACE(TOPIC_TRIGGER_TIMEOUT, {
    task_id id   = as_uval(v);
    poll_t *poll = (poll_t *)tidmap_find(&_state.polls, id);
    if (poll == NULL) {
        return;
    }
    _cleanup_poll(poll);
    tidmap_deregister(&_state.polls, id);
})

/*******************************************************************************
 * utils
 ******************************************************************************/
static void
_fd_init(tiditem_t *item)
{
    tidset_init(&((fd_t *)item)->waiters);
}

static void
_wait(task_id tid, poll_args_t *args)
{
    poll_t *poll = (poll_t *)tidmap_register(&_state.polls, tid);
    ASSERT(poll);
    poll->args  = args;
    int timeout = poll->args->timeout;
    if (timeout != 0 && timeout != -1) {
        struct timespec deadline;
        clock_time(&deadline);
        uint64_t timeout_ns  = timeout * NSEC_IN_MSEC;
        uint64_t deadline_ns = deadline.tv_nsec + timeout_ns;
        deadline.tv_sec += (long)(deadline_ns / NSEC_IN_SEC);
        deadline.tv_nsec = (long)(deadline_ns % NSEC_IN_SEC);
        handler_timeout_register(tid, &deadline);
    }
    for (nfds_t i = 0; i < args->nfds; i++) {
        struct pollfd *fds = args->fds + i;
        fds->revents       = 0;
        int fd             = fds->fd;
        if (fd < 0) {
            continue;
        }
        // add error events for correct fd bookkeeping
        short events = fds->events |= POLLERR | POLLHUP | POLLNVAL;
        fd_t *fde =
            (fd_t *)tidmap_find_or_register(&_state.fds, fd + 1, _fd_init);
        fde->events = (short)(fde->events | events);
        tidset_insert(&fde->waiters, tid);
        uint64_t j = 0;
        for (short i = 1; i; i <<= 1, j++) {
            if ((i & events) == 0) {
                continue;
            }
            fde->counts[j]++;
        }
    }
}

static void
_propagate_events(struct pollfd *fds, nfds_t nfds, int nevents)
{
    for (nfds_t i = 0; i < nfds; i++) {
        struct pollfd *pollfd = fds + i;
        short revents         = pollfd->revents;
        if (revents == 0) {
            continue;
        }
        nevents--;
        int fd    = pollfd->fd;
        fd_t *fde = (fd_t *)tidmap_find(&_state.fds, fd + 1);
        ASSERT(fde);
        tidset_t *waiters = &fde->waiters;
        size_t size       = tidset_size(waiters);
        for (size_t i = 0; i < size; i++) {
            poll_t *poll =
                (poll_t *)tidmap_find(&_state.polls, tidset_get(waiters, i));
            ASSERT(poll);
            poll_args_t *args = poll->args;
            for (nfds_t k = 0; k < args->nfds; k++) {
                struct pollfd *pollfd = args->fds + k;
                if (pollfd->fd != fd ||
                    (pollfd->revents = (short)(pollfd->revents |
                                               (pollfd->events & revents))) ==
                        0) {
                    continue;
                }
                args->ret++;
            }
        }
    }
    ASSERT(nevents == 0);
}

static void
_cleanup_poll(const poll_t *poll)
{
    poll_args_t *args = poll->args;
    nfds_t ndfs       = args->nfds;
    task_id id        = poll->ti.key;
    for (nfds_t i = 0; i < ndfs; i++) {
        struct pollfd *pollfd = args->fds + i;
        fd_t *fd = (fd_t *)tidmap_find(&_state.fds, pollfd->fd + 1);
        ASSERT(fd);
        size_t k = 0;
        for (short j = 1; j; j <<= 1, k++) {
            if ((j & pollfd->events) == 0) {
                continue;
            }
            fd->counts[k]--;
        }
        tidset_remove(&fd->waiters, id);
    }
}

static void
_cleanup_polls()
{
    for (const tiditem_t *cur = tidmap_iterate(&_state.polls); cur;) {
        poll_t *poll      = (poll_t *)cur;
        poll_args_t *args = poll->args;
        nfds_t ndfs       = args->nfds;
        int timeout       = args->timeout;
        if (args->ret == 0 && ndfs > 0 && timeout != 0) {
            cur = tidmap_next(cur);
            continue;
        }
        _cleanup_poll(poll);
        task_id id = cur->key;

        handler_timeout_deregister(id);
        cur = tidmap_next(cur);
        tidmap_deregister(&_state.polls, id);
    }
}

static void
_cleanup_fds()
{
    for (const tiditem_t *cur = tidmap_iterate(&_state.fds); cur;) {
        fd_t *fd    = (fd_t *)cur;
        bool remove = true;
        for (size_t i = 0; i < sizeof(short) * 8 && remove; i++) {
            remove = remove && fd->counts[i] == 0;
        }
        if (!remove) {
            cur = tidmap_next(cur);
            continue;
        }
        task_id id        = cur->key;
        cur               = tidmap_next(cur);
        tidset_t *waiters = &fd->waiters;
        ASSERT(tidset_size(waiters) == 0);
        tidset_fini(waiters);
        tidmap_deregister(&_state.fds, id);
    }
}

static void
_cleanup()
{
    _cleanup_polls();
    _cleanup_fds();
}

static void
_poll()
{
    nfds_t nfds = tidmap_size(&_state.fds);
    if (nfds == 0) {
        return;
    }
    struct pollfd *fds = alloca(nfds * sizeof(struct pollfd));
    nfds_t i           = 0;
    for (const tiditem_t *cur = tidmap_iterate(&_state.fds); cur;
         cur                  = tidmap_next(cur)) {
        fd_t *fd = (fd_t *)cur;
        fds[i++] =
            (struct pollfd){.fd = (int)cur->key - 1, .events = fd->events};
    }
    int nevents = sys_poll(fds, nfds, 0);
    if (nevents == 0) {
        return;
    }
    _propagate_events(fds, nfds, nevents);
}

static bool
_should_wait(task_id id)
{
    return tidmap_find(&_state.polls, id) != NULL;
}

/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_poll_handle(const context_t *ctx, event_t *e)
{
    ASSERT(ctx);
    ASSERT(e);
    if (_state.should_cleanup) {
        _state.should_cleanup = false;
        _cleanup_fds();
    }
    switch (ctx->cat) {
        case CAT_POLL:
            _wait(ctx->id, (poll_args_t *)ctx->args[0].value.ptr);
            break;
        default:
            break;
    }
    _poll();
    _cleanup();
    tidset_remove_all_keys(&e->tset, &_state.polls);
    if (_should_wait(ctx->id)) {
        e->is_chpt = true;
        ASSERT(e->any_task_filter == NULL);
        e->any_task_filter = _should_wait;
    }
}
REGISTER_HANDLER(SLOT_POLL, _poll_handle)
