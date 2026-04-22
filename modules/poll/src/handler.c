
#include "poll.h"
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/clock.h>
#include <lotto/modules/poll/events.h>
#include <lotto/modules/timeout/timeout.h>
#include <lotto/sys/poll.h>

#define POLL_HELP_INTERVAL 8

/*******************************************************************************
 * state
 ******************************************************************************/
struct poll_waiter {
    tiditem_t ti;
    struct poll_event *args;
};

struct polled_fd {
    tiditem_t ti;
    short events;
    tidset_t waiters;
    uint64_t counts[sizeof(short) * 8]; // a counter per events bit
};

static struct handler_poll {
    tidmap_t polls;
    tidmap_t fds;
    uint64_t help_counter;
    bool should_cleanup;
} _state;

REGISTER_STATE(EPHEMERAL, _state, {
    tidmap_init(&_state.polls, MARSHABLE_STATIC(sizeof(struct poll_waiter)));
    tidmap_init(
        &_state.fds,
        MARSHABLE_STATIC(sizeof(struct polled_fd))); // no marshal support
})
static void _cleanup_poll(const struct poll_waiter *waiter);
static void _cleanup_fds();
/* Unblock task when timeout is triggered by the timeout handler */
LOTTO_SUBSCRIBE(EVENT_TIMEOUT_TRIGGER, {
    task_id id = as_uval(v);
    struct poll_waiter *waiter =
        (struct poll_waiter *)tidmap_find(&_state.polls, id);
    if (waiter == NULL) {
        return PS_OK;
    }
    _cleanup_poll(waiter);
    tidmap_deregister(&_state.polls, id);
})

/*******************************************************************************
 * utils
 ******************************************************************************/
static void
_fd_init(tiditem_t *item)
{
    tidset_init(&((struct polled_fd *)item)->waiters);
}

static void
_wait(task_id tid, struct poll_event *args)
{
    struct poll_waiter *waiter =
        (struct poll_waiter *)tidmap_register(&_state.polls, tid);
    ASSERT(waiter);
    waiter->args = args;
    int timeout  = waiter->args->timeout;
    if (timeout != 0 && timeout != -1) {
        struct timespec deadline;
        lotto_clock_time(&deadline);
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
        struct polled_fd *polled_fd =
            (struct polled_fd *)tidmap_find_or_register(&_state.fds, fd + 1,
                                                        _fd_init);
        polled_fd->events = (short)(polled_fd->events | events);
        tidset_insert(&polled_fd->waiters, tid);
        uint64_t j = 0;
        for (short i = 1; i; i <<= 1, j++) {
            if ((i & events) == 0) {
                continue;
            }
            polled_fd->counts[j]++;
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
        int fd = pollfd->fd;
        struct polled_fd *polled_fd =
            (struct polled_fd *)tidmap_find(&_state.fds, fd + 1);
        ASSERT(polled_fd);
        tidset_t *waiters = &polled_fd->waiters;
        size_t size       = tidset_size(waiters);
        for (size_t i = 0; i < size; i++) {
            struct poll_waiter *waiter = (struct poll_waiter *)tidmap_find(
                &_state.polls, tidset_get(waiters, i));
            ASSERT(waiter);
            struct poll_event *args = waiter->args;
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
_cleanup_poll(const struct poll_waiter *waiter)
{
    struct poll_event *args = waiter->args;
    nfds_t ndfs             = args->nfds;
    task_id id              = waiter->ti.key;
    for (nfds_t i = 0; i < ndfs; i++) {
        struct pollfd *pollfd = args->fds + i;
        struct polled_fd *polled_fd =
            (struct polled_fd *)tidmap_find(&_state.fds, pollfd->fd + 1);
        ASSERT(polled_fd);
        size_t k = 0;
        for (short j = 1; j; j <<= 1, k++) {
            if ((j & pollfd->events) == 0) {
                continue;
            }
            polled_fd->counts[k]--;
        }
        tidset_remove(&polled_fd->waiters, id);
    }
}

static void
_cleanup_polls()
{
    for (const tiditem_t *cur = tidmap_iterate(&_state.polls); cur;) {
        struct poll_waiter *waiter = (struct poll_waiter *)cur;
        struct poll_event *args    = waiter->args;
        nfds_t ndfs                = args->nfds;
        int timeout                = args->timeout;
        if (args->ret == 0 && ndfs > 0 && timeout != 0) {
            cur = tidmap_next(cur);
            continue;
        }
        _cleanup_poll(waiter);
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
        struct polled_fd *polled_fd = (struct polled_fd *)cur;
        bool remove                 = true;
        for (size_t i = 0; i < sizeof(short) * 8 && remove; i++) {
            remove = remove && polled_fd->counts[i] == 0;
        }
        if (!remove) {
            cur = tidmap_next(cur);
            continue;
        }
        task_id id        = cur->key;
        cur               = tidmap_next(cur);
        tidset_t *waiters = &polled_fd->waiters;
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
        struct polled_fd *polled_fd = (struct polled_fd *)cur;
        fds[i++]                    = (struct pollfd){.fd     = (int)cur->key - 1,
                                                      .events = polled_fd->events};
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
_poll_register_waiter(const capture_point *cp, event_t *e)
{
    ASSERT(cp);
    ASSERT(e);
    if (cp->type_id == EVENT_POLL) {
        _wait(cp->id, (struct poll_event *)cp->payload);
    }
}

STATIC void
_poll_help_waiters(const capture_point *cp, sequencer_decision *e)
{
    ASSERT(cp);
    ASSERT(e);
    if (_state.should_cleanup) {
        _state.should_cleanup = false;
        _cleanup_fds();
    }
    _state.help_counter++;
    if ((_state.help_counter % POLL_HELP_INTERVAL) == 0) {
        _poll();
    }
    _cleanup();
    tidset_remove_all_keys(&e->tset, &_state.polls);
    if (_should_wait(cp->id)) {
        e->is_chpt = true;
        ASSERT(e->any_task_filter == NULL);
        e->any_task_filter = _should_wait;
    }
}

LOTTO_SUBSCRIBE_SEQUENCER_CAPTURE(EVENT_POLL, {
    const capture_point *cp = EVENT_PAYLOAD(cp);
    sequencer_decision *e   = cp->decision;
    ASSERT(cp);
    ASSERT(e);
    _poll_register_waiter(cp, e);
})

LOTTO_SUBSCRIBE_SEQUENCER_CAPTURE(ANY_EVENT, {
    const capture_point *cp = EVENT_PAYLOAD(cp);
    sequencer_decision *e   = cp->decision;
    ASSERT(cp);
    ASSERT(e);
    _poll_help_waiters(cp, e);
})

bool
lotto_dbg_poll_is_waiting(task_id id)
{
    return tidmap_find(&_state.polls, id) != NULL;
}

const tidset_t *
lotto_dbg_poll_waiters(void)
{
    static tidset_t waiters = {0};
    static bool initialized = false;
    if (!initialized) {
        tidset_init(&waiters);
        initialized = true;
    }
    tidset_clear(&waiters);
    for (const tiditem_t *cur = tidmap_iterate(&_state.polls); cur;
         cur                  = tidmap_next(cur)) {
        tidset_insert(&waiters, cur->key);
    }
    return &waiters;
}
