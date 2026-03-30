#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE

#include "sighandler.h"
#include <dice/events/self.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/base/clk.h>
#include <lotto/base/trace_file.h>
#include <lotto/engine/engine.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/runtime/mediator.h>
#include <lotto/runtime/runtime.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/memory.h>
#include <lotto/sys/now.h>
#include <lotto/sys/real.h>
#include <lotto/sys/stream_file.h>
#include <vsync/atomic/core.h>
#include <vsync/atomic/dispatch.h>

static nanosec_t _start;
static trace_t *_recorder;
static trace_t *_replayer;
static int _logger_fd = -1;

typedef struct {
    const capture_point *cp;
    reason_t reason;
} cb_data;

static vatomic64_t _only_once;

bool
_lotto_loaded(void)
{
    /* Inform user that the Lotto runtime library is loaded. */
    return true;
}

static void
runtime_init_(void)
{
    const char *var = getenv("LOTTO_DISABLE");
    if (var) {
        return;
    }

    _start = now();
    if ((var = getenv("LOTTO_RECORD")) && var[0]) {
        logger_debugf("record: %s\n", var);
        stream_t *st = stream_file_alloc();
        stream_file_out(st, var);
        _recorder = trace_file_create(st);
    } else {
        stream_t *st = stream_file_alloc();
        stream_file_out(st, "/dev/null");
        _recorder = trace_file_create(st);
    }

    if ((var = getenv("LOTTO_REPLAY")) && var[0]) {
        logger_debugf("replay: %s\n", var);
        stream_t *st = stream_file_alloc();
        stream_file_in(st, var);
        _replayer = trace_file_create(st);
    } else {
        _replayer = NULL;
    }

    sighandler_init();
    engine_init(_replayer, _recorder);
}

static void
logger_init_(void)
{
    const char *var = getenv("LOTTO_LOGGER_LEVEL");

    enum logger_level level = LOGGER_ERROR;
    if (var) {
        if (strcmp(var, "debug") == 0)
            level = LOGGER_DEBUG;
        else if (strcmp(var, "info") == 0)
            level = LOGGER_INFO;
        else if (strcmp(var, "warn") == 0)
            level = LOGGER_WARN;
        else if (strcmp(var, "silent") == 0) {
            logger(level, -1);
            return;
        }
    }

    var = getenv("LOTTO_LOGGER_FILE");
    if (var) {
        if (strcmp(var, "stdout") == 0)
            logger(level, STDOUT_FILENO);
        else if (strcmp(var, "stderr") == 0)
            logger(level, STDERR_FILENO);
        else {
            _logger_fd = open(var, O_WRONLY | O_CREAT | O_APPEND, 0644);
            ASSERT(_logger_fd >= 0);
            logger(level, _logger_fd);
        }
    } else {
        logger(level, STDOUT_FILENO);
    }
}

static void *
fini_cb_(void *arg)
{
    const cb_data *dat = (const cb_data *)arg;
    ASSERT(dat != NULL);
    int err = 0;
    if (vatomic_xchg(&_only_once, 1) == 0) {
        mediator_t *m = self_md() ? mediator_get(self_md(), false) : NULL;
        if (m)
            mediator_fini(m);

        err = engine_fini(dat->cp, dat->reason);

        if (_recorder) {
            stream_close(trace_stream(_recorder));
        }

        if (_replayer) {
            stream_close(trace_stream(_replayer));
        }

        lotto_intercept_fini();
        sys_memory_fini();
        nanosec_t elapsed = now() - _start;
        logger_debugf("Elapsed time = %.2fs\n", in_sec(elapsed));
        _exit(err);
    }
    REAL_DECL(void, pthread_exit, void *arg);
    REAL_NAME(pthread_exit) = real_func("pthread_exit", 0);
    REAL(pthread_exit, 0);

    return NULL;
}

NORETURN void
lotto_exit(capture_point *cp, reason_t reason)
{
    fflush(stdout);
    fflush(stderr);
    cp->id      = (cp->id == NO_TASK) ? 1 : cp->id;
    cb_data dat = {
        .cp     = cp,
        .reason = reason,
    };
    fini_cb_(&dat);
    logger_fatalf("should never reach here");
    sys_abort();
}

static void DICE_DTOR
runtime_fini_(void)
{
    if (_logger_fd >= 0) {
        close(_logger_fd);
        _logger_fd = -1;
    }

    capture_point cp = {
        .chain_id = 0,
        .type_id  = EVENT_TASK_FINI,
        .id       = 1,
        .vid      = NO_TASK,
        .func     = __FUNCTION__,
    };
    lotto_exit(&cp, REASON_SUCCESS);
}

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SELF_WAIT, {
    static uint64_t cnt        = 0;
    struct self_wait_event *ev = EVENT_PAYLOAD(event);

    if (++cnt < 10) {
        ev->wait = true;
    }
})

ON_INITIALIZATION_PHASE({
    runtime_init_();
    logger_init_();
})

LOTTO_MODULE_INIT()
