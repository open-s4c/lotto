// clang-format off
#if defined(__linux__) && !defined(FUTEX_USERSPACE)
#undef __linux__
#include <vsync/thread/internal/futex.h>
#define __linux__
#endif
// clang-format on

#include "state.h"
#include <dice/module.h>
#include <dice/self.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/workgroup/events.h>
#include <lotto/runtime/events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/string.h>
#include <vsync/spinlock/caslock.h>

typedef struct workgroup_tls_state {
    bool in_workgroup;
} workgroup_tls_state_t;

typedef struct workgroup_runtime_state {
    caslock_t lock;
    bool concurrent_phase;
    bool main_registered;
    uint64_t pending_starts;
    tidset_t started;
    tidset_t waiting;
} workgroup_runtime_state_t;

static workgroup_config_t _config = {
    .thread_start_policy = WORKGROUP_THREAD_START_POLICY_SEQUENTIAL,
};

static workgroup_runtime_state_t _runtime = {
    .concurrent_phase = false,
};

static workgroup_tls_state_t _tls;

static bool
_debug_enabled(void)
{
    return getenv("LOTTO_WORKGROUP_DEBUG") != NULL;
}

static void
_debug_state(const char *tag, task_id id)
{
    if (!_debug_enabled()) {
        return;
    }

    fprintf(stderr,
            "workgroup:%s tid=%lu concurrent=%d pending=%lu started=%lu waiting=%lu\n",
            tag, id, _runtime.concurrent_phase, _runtime.pending_starts,
            tidset_size(&_runtime.started), tidset_size(&_runtime.waiting));
}

static void
_workgroup_config_print(const marshable_t *m)
{
    const workgroup_config_t *cfg = (const workgroup_config_t *)m;
    logger_infof("thread_start_policy = %s\n",
                 workgroup_thread_start_policy_str(cfg->thread_start_policy));
}

REGISTER_CONFIG_NONSTATIC(_config,
                          MARSHABLE_STATIC_PRINTABLE(sizeof(_config),
                                                     _workgroup_config_print))

ON_INITIALIZATION_PHASE({
    _runtime.concurrent_phase =
        _config.thread_start_policy == WORKGROUP_THREAD_START_POLICY_CONCURRENT;
    caslock_init(&_runtime.lock);
    _runtime.main_registered = false;
    _runtime.pending_starts  = 0;
    tidset_init(&_runtime.started);
    tidset_init(&_runtime.waiting);
})

static inline workgroup_tls_state_t *
_tls_get(metadata_t *md)
{
    if (md == NULL) {
        return NULL;
    }
    return SELF_TLS(md, &_tls);
}

static inline bool
_type_allowed(type_id type)
{
    return type == EVENT_TASK_INIT || type == EVENT_TASK_CREATE ||
           type == EVENT_TASK_JOIN || type == EVENT_TASK_FINI ||
           type == EVENT_WORKGROUP_JOIN;
}

workgroup_config_t *
workgroup_config(void)
{
    return &_config;
}

const char *
workgroup_thread_start_policy_str(uint64_t bits)
{
    switch ((workgroup_thread_start_policy_t)bits) {
        case WORKGROUP_THREAD_START_POLICY_SEQUENTIAL:
            return "SEQUENTIAL";
        case WORKGROUP_THREAD_START_POLICY_CONCURRENT:
            return "CONCURRENT";
        default:
            ASSERT(false && "unexpected workgroup thread-start policy");
            return "SEQUENTIAL";
    }
}

uint64_t
workgroup_thread_start_policy_from(const char *src)
{
    if (sys_strcmp(src, "SEQUENTIAL") == 0) {
        return WORKGROUP_THREAD_START_POLICY_SEQUENTIAL;
    }
    if (sys_strcmp(src, "CONCURRENT") == 0) {
        return WORKGROUP_THREAD_START_POLICY_CONCURRENT;
    }
    ASSERT(false && "invalid workgroup thread-start policy");
    return WORKGROUP_THREAD_START_POLICY_SEQUENTIAL;
}

void
workgroup_thread_start_policy_all_str(char *dst)
{
    sys_strcpy(dst, "SEQUENTIAL|CONCURRENT");
}

bool
workgroup_should_filter(metadata_t *md, chain_id chain, type_id type)
{
    (void)chain;
    bool concurrent_phase;

    caslock_acquire(&_runtime.lock);
    concurrent_phase = _runtime.concurrent_phase;
    caslock_release(&_runtime.lock);

    if (_type_allowed(type)) {
        return false;
    }

    workgroup_tls_state_t *tls = _tls_get(md);
    if (tls == NULL || !tls->in_workgroup) {
        return false;
    }
    return concurrent_phase;
}

void
workgroup_on_task_init(metadata_t *md)
{
    workgroup_tls_state_t *tls = _tls_get(md);
    if (tls == NULL) {
        return;
    }

    caslock_acquire(&_runtime.lock);
    bool concurrent_phase = _runtime.concurrent_phase;
    caslock_release(&_runtime.lock);

    if (!concurrent_phase || tls->in_workgroup) {
        return;
    }

    tls->in_workgroup = true;
}

void
workgroup_on_task_fini(metadata_t *md)
{
    workgroup_tls_state_t *tls = _tls_get(md);
    if (tls == NULL) {
        return;
    }

    tls->in_workgroup = false;
}

void
workgroup_clear_current(metadata_t *md)
{
    workgroup_tls_state_t *tls = _tls_get(md);
    if (tls == NULL) {
        return;
    }
    tls->in_workgroup = false;
}

static bool
_should_wait(task_id id)
{
    bool should_wait;

    caslock_acquire(&_runtime.lock);
    should_wait = tidset_has(&_runtime.waiting, id);
    caslock_release(&_runtime.lock);

    return should_wait;
}

static void
_release_group(void)
{
    _runtime.concurrent_phase = false;
    _runtime.main_registered  = false;
    _runtime.pending_starts   = 0;
    tidset_clear(&_runtime.started);
}

void
workgroup_handle_capture(const capture_point *cp, event_t *e)
{
    caslock_acquire(&_runtime.lock);

    if (!_runtime.concurrent_phase && tidset_size(&_runtime.waiting) == 0) {
        caslock_release(&_runtime.lock);
        return;
    }

    switch (cp->type_id) {
        case EVENT_TASK_INIT:
            if (_runtime.concurrent_phase) {
                if (cp->id == MAIN_THREAD && !_runtime.main_registered) {
                    _runtime.main_registered = true;
                    tidset_insert(&_runtime.started, cp->id);
                } else if (_runtime.pending_starts > 0) {
                    tidset_insert(&_runtime.started, cp->id);
                    _runtime.pending_starts--;
                }
            }
            _debug_state("task-init", cp->id);
            break;
        case EVENT_TASK_CREATE:
            if (_runtime.concurrent_phase &&
                cp->chain_id == CHAIN_INGRESS_AFTER && cp->task_create != NULL &&
                cp->task_create->ret == 0 &&
                tidset_has(&_runtime.started, cp->id)) {
                _runtime.pending_starts++;
            }
            _debug_state("task-create", cp->id);
            break;
        case EVENT_TASK_FINI:
            tidset_remove(&_runtime.started, cp->id);
            tidset_remove(&_runtime.waiting, cp->id);
            if (_runtime.concurrent_phase &&
                tidset_size(&_runtime.started) == 0 &&
                _runtime.pending_starts == 0) {
                _release_group();
            }
            _debug_state("task-fini", cp->id);
            break;
        case EVENT_WORKGROUP_JOIN:
            if (!_runtime.concurrent_phase) {
                _debug_state("join-post-release", cp->id);
                break;
            }
            tidset_insert(&_runtime.waiting, cp->id);
            tidset_remove(&_runtime.started, cp->id);
            if (tidset_size(&_runtime.started) == 0 &&
                _runtime.pending_starts == 0) {
                _release_group();
            }
            e->is_chpt = true;
            if (e->any_task_filter == NULL) {
                e->any_task_filter = _should_wait;
            }
            _debug_state("join", cp->id);
            break;
        default:
            break;
    }

    if (_runtime.concurrent_phase && tidset_size(&_runtime.waiting) > 0) {
        tidset_subtract(&e->tset, &_runtime.waiting);
    } else if (tidset_size(&_runtime.waiting) > 0) {
        tidset_copy(&e->tset, &_runtime.waiting);
    }
    if (cp->type_id == EVENT_WORKGROUP_JOIN && _runtime.concurrent_phase &&
        tidset_has(&_runtime.waiting, cp->id)) {
        e->reason  = REASON_BLOCKING;
        e->is_chpt = true;
    }

    caslock_release(&_runtime.lock);
}

void
workgroup_handle_resume(const capture_point *cp)
{
    if (cp->type_id != EVENT_WORKGROUP_JOIN) {
        return;
    }

    caslock_acquire(&_runtime.lock);
    tidset_remove(&_runtime.waiting, cp->id);
    _debug_state("resume", cp->id);
    caslock_release(&_runtime.lock);
}
