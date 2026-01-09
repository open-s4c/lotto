#include <stdlib.h>

#include "interceptor.h"
#include <dice/module.h>
#include <lotto/base/context.h>
#include <lotto/base/reason.h>
#include <lotto/base/trace_file.h>
#include <lotto/engine/engine.h>
#include <lotto/runtime/runtime.h>
#include <lotto/sys/stream_file.h>

static nanosec_t _start;
static trace_t *_recorder;
static trace_t *_replayer;
static FILE *_logger_fp;

void
sighandler_init(void)
{
}

void LOTTO_CONSTRUCTOR
lotto_runtime_init(void)
{
    const char *var = getenv("LOTTO_DISABLE");
    if (var) {
        return;
    }

    _start = now();
    /* prepare engine configuration and initilize engine */
    if ((var = getenv("LOTTO_RECORD")) && var[0]) {
        logger_debugf("record: %s\n", var);
        stream_t *st = stream_file_alloc();
        stream_file_out(st, var);
        _recorder = trace_file_create(st);
    } else {
        /* To guarantee the number of file descriptors is the same on
         * replay, we have to open a file, even when not recording. */
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


    /* initialize subcomponents */
    // sighandler_init();
    engine_init(_replayer, _recorder);
}

void
lotto_exit(context_t *ctx, reason_t reason)
{
    int status = engine_fini(ctx, reason);
    exit(status);
}
