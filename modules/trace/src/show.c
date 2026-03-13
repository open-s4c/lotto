#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/trace.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/record.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/trace.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

int
show(args_t *args, flags_t *flags)
{
    logger(LOGGER_INFO, STDOUT_FILENO);

    const char *fn = flags_get_sval(flags, flag_input());
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

LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_INIT, {
    flag_t sel[] = {flag_input(), flag_temporary_directory(),
                    flag_no_preload(), flag_verbose(), 0};
    subcmd_register(show, "show", "", "Show details of a trace", false, sel,
                    flags_default, SUBCMD_GROUP_TRACE);
})
