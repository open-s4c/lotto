#include <lotto/base/trace.h>
#include <lotto/base/trace_file.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/record.h>
#include <lotto/driver/subcmd.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream.h>
#include <lotto/sys/stream_file.h>

int
show(args_t *args, flags_t *flags)
{
    (void)args;
    logger(LOGGER_INFO, STDOUT_FILENO);

    const char *fn = flags_get_sval(flags, flag_input());
    sys_fprintf(stdout, "trace file: %s\n", fn);

    stream_t *st = stream_file_alloc();
    if (st == NULL) {
        sys_fprintf(stderr, "show error: could not allocate stream\n");
        return 1;
    }

    trace_t *rec = NULL;
    if (fn && fn[0]) {
        stream_file_in(st, fn);
        rec = trace_file_create(st);
    }
    if (rec == NULL) {
        sys_free(st);
        sys_fprintf(stderr, "show error: could not open %s\n", fn);
        return 1;
    }

    record_t *cur;
    int i = 0;
    while ((cur = trace_next(rec, RECORD_ANY)) != NULL) {
        record_print(cur, i++);
        trace_advance(rec);
    }
    stream_close(st);
    trace_destroy(rec);
    sys_free(st);

    return 0;
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_input(), flag_temporary_directory(), flag_no_preload(),
                    flag_verbose(), 0};
    subcmd_register(show, "show", "", "Show details of a trace", false, sel,
                    flags_default, SUBCMD_GROUP_TRACE);
})
