/*
 */
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include "interceptor_internal.h"
#include <lotto/base/clk.h>
#include <lotto/base/trace_file.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/pubsub_interface.h>
#include <lotto/engine/engine.h> // for engine_init
#include <lotto/runtime/intercept.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/memory.h>
#include <lotto/sys/now.h>
#include <lotto/sys/real.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stream_chunked_file.h>
#include <lotto/sys/stream_file.h>
#include <sys/stat.h>
#include <vsync/atomic/core.h>
#include <vsync/atomic/dispatch.h>

static nanosec_t _start;
static trace_t *_recorder;
static trace_t *_replayer;
static FILE *_logger_fp;

void sighandler_init(void);

static void LOTTO_CONSTRUCTOR
_runtime_init()
{
    lotto_set_interceptor_initialized();

    const char *var = getenv("LOTTO_DISABLE");
    if (var) {
        return;
    }

    _start = now();
    /* prepare engine configuration and initilize engine */
    if ((var = getenv("LOTTO_RECORD")) && var[0]) {
        log_debugf("record: %s\n", var);
        stream_t *st = stream_file_alloc();
        stream_file_out(st, var);
        _recorder = trace_file_create(st);
    } else {
        /* To guarantee the number of file descriptors is the same on replay, we
         * have to open a file, even when not recording. */
        stream_t *st = stream_file_alloc();
        stream_file_out(st, "/dev/null");
        _recorder = trace_file_create(st);
    }

    if ((var = getenv("LOTTO_REPLAY")) && var[0]) {
        log_debugf("replay: %s\n", var);
        stream_t *st = stream_file_alloc();
        stream_file_in(st, var);
        _replayer = trace_file_create(st);
    } else {
        _replayer = NULL;
    }


    /* initialize subcomponents */
    sighandler_init();
    engine_init(_replayer, _recorder);
}

static void LOTTO_CONSTRUCTOR
_log_init()
{
    const char *var = getenv("LOTTO_LOG_LEVEL");

    enum log_level level = LOG_ERROR;
    FILE *null           = sys_fopen("/dev/null", "a+");
    if (var) {
        if (strcmp(var, "debug") == 0)
            level = LOG_DEBUG;
        else if (strcmp(var, "info") == 0)
            level = LOG_INFO;
        else if (strcmp(var, "warn") == 0)
            level = LOG_WARN;
        else if (strcmp(var, "silent") == 0) {
            logger(level, null);
            return;
        }
    }

    var = getenv("LOTTO_LOG_FILE");
    if (var) {
        if (strcmp(var, "stdout") == 0)
            logger(level, stdout);
        else if (strcmp(var, "stderr") == 0)
            logger(level, stderr);
        else {
            _logger_fp = sys_fopen(var, "a+");
            ASSERT(NULL != _logger_fp);
            logger(level, _logger_fp);
        }
    } else {
        logger(level, NULL);
    }

    var = getenv("LOTTO_LOGGER_BLOCK");
    if (var) {
        logger_block_init((char *)var);
    }
}

typedef struct {
    const context_t *ctx;
    reason_t reason;
} cb_data;

static vatomic64_t _only_once;

static void *
_fini_cb(void *arg)
{
    const cb_data *dat = (const cb_data *)arg;
    ASSERT(dat != NULL);
    int err = 0;
    if (vatomic_xchg(&_only_once, 1) == 0) {
        mediator_t *m = get_mediator(false);
        mediator_fini(m);

        err = engine_fini(dat->ctx, dat->reason);

        if (_recorder) {
            stream_close(trace_stream(_recorder));
        }

        if (_replayer) {
            stream_close(trace_stream(_replayer));
        }

        lotto_intercept_fini();
        logger_block_fini();
        sys_memory_fini();
        nanosec_t elapsed = now() - _start;
        log_debugf("Elapsed time = %.2fs\n", in_sec(elapsed));
        _exit(err);
    }
    REAL_DECL(void, pthread_exit, void *arg);
    REAL_NAME(pthread_exit) = real_func("pthread_exit", 0);
    REAL(pthread_exit, 0);

    return NULL;
}

NORETURN void
lotto_exit(context_t *ctx, reason_t reason)
{
    fflush(stdout);
    fflush(stderr);
    ctx->id     = (ctx->id == NO_TASK) ? get_task_id() : ctx->id;
    cb_data dat = {
        .ctx    = ctx,
        .reason = reason,
    };
    _fini_cb(&dat);
    log_fatalf("should never reach here");
    sys_abort();
}

static void LOTTO_DESTRUCTOR
_runtime_fini()
{
    if (_logger_fp) {
        sys_fclose(_logger_fp);
        _logger_fp = NULL;
    }

    context_t ctx = {
        .id  = 1, /* main task */
        .cat = CAT_TASK_FINI,
    };
    lotto_exit(&ctx, REASON_SUCCESS);
}
