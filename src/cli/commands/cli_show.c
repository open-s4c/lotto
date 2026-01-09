#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/trace.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

DECLARE_FLAG_INPUT;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_VERBOSE;

int
show(args_t *args, flags_t *flags)
{
    logger(LOG_INFO, stdout);

    const char *fn = flags_get_sval(flags, FLAG_INPUT);
    sys_fprintf(stdout, "trace file: %s\n", fn);

    trace_t *rec = cli_trace_load(fn);
    if (rec == NULL) {
        sys_fprintf(stderr, "show error: could not open %s\n", fn);
        return 1;
    }

    record_t *cur;
    int i = 0;
    while ((cur = trace_next(rec, RECORD_ANY)) != NULL) {
        record_print(cur, i++);
        trace_advance(rec);
    }
    trace_destroy(rec);
    return 0;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_INPUT, FLAG_TEMPORARY_DIRECTORY, FLAG_NO_PRELOAD,
                    FLAG_VERBOSE, 0};
    subcmd_register(show, "show", "", "Show details of a trace", false, sel,
                    flags_default, SUBCMD_GROUP_TRACE);
}
