/*******************************************************************************
 * replay
 ******************************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/envvar.h>
#include <lotto/base/record_granularity.h>
#include <lotto/cli/preload.h>
#include <lotto/driver/exec.h>
#include <lotto/driver/exec_info.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/memmgr.h>
#include <lotto/driver/flags/sequencer.h>
#include <lotto/driver/record.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/trace.h>
#include <lotto/driver/utils.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/stdio.h>

int
replay(args_t *args, flags_t *flags)
{
    setenv("LOTTO_LOGGER_FILE", flags_get_sval(flags, flag_logger_file()),
           true);

    sys_fprintf(stdout, "trace file: %s\n", flags_get_sval(flags, flag_input()));

    uint64_t last_clk = cli_trace_last_clk(flags_get_sval(flags, flag_input()));
    uint64_t goal     = flags_get_uval(flags, flag_replay_goal());
    if (goal > last_clk) {
        flags_set_default(flags, flag_replay_goal(), uval(last_clk));
    }

    const char *input_fn = flags_get_sval(flags, flag_input());
    trace_t *rec         = cli_trace_load(input_fn);
    record_t *first      = trace_next(rec, RECORD_START);
    if (first == NULL) {
        sys_fprintf(stderr, "error: empty trace file '%s'\n", input_fn);
        return 1;
    }

    args = record_args(first);

    preload(flags_get_sval(flags, flag_temporary_directory()),
            flags_is_on(flags, flag_verbose()),
            !flags_is_on(flags, flag_no_preload()),
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    trace_advance(rec);
    statemgr_record_unmarshal(trace_next(rec, RECORD_CONFIG));
    trace_destroy(rec);

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = flags_get_sval(flags, flag_input())},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, flag_logger_block())},
        {NULL}};
    envvar_set(vars, true);

    const char *output_fn = flags_get_sval(flags, flag_output());
    if (output_fn && output_fn[0]) {
        envvar_t vars[] = {{"LOTTO_RECORD", .sval = output_fn}, {NULL}};
        envvar_set(vars, true);
    }
    record_granularities_t record_granularity =
        flags_get_uval(flags, flag_record_granularity());
    char var[RECORD_GRANULARITIES_MAX_LEN];
    record_granularities_str(record_granularity, var);
    setenv("LOTTO_RECORD_GRANULARITY", var, true);
    if (flags_is_on(flags, flag_verbose())) {
        sys_fprintf(stdout, "[lotto] replaying: ");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }
    adjust(flags_get_sval(flags, flag_input()));
    return execute(args, flags, true);
}

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_output(), sval(""));
    return flags;
}

LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__REGISTER_COMMANDS, {
    flag_t sel[] = {flag_input(),
                    flag_output(),
                    flag_verbose(),
                    flag_replay_goal(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_logger_block(),
                    flag_before_run(),
                    flag_after_run(),
                    flag_logger_file(),
                    0};
    subcmd_register(replay, "replay", "", "Replay a trace", true, sel,
                    _default_flags, SUBCMD_GROUP_TRACE);
})
