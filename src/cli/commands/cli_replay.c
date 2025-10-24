/*
 */
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
#include <lotto/brokers/statemgr.h>
#include <lotto/cli/exec.h>
#include <lotto/cli/exec_info.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/memmgr.h>
#include <lotto/cli/flags/sequencer.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/cli/utils.h>
#include <lotto/sys/stdio.h>

DECLARE_FLAG_CREP;
DECLARE_FLAG_INPUT;
DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_REPLAY_GOAL;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_LOGGER_BLOCK;
DECLARE_FLAG_BEFORE_RUN;
DECLARE_FLAG_AFTER_RUN;
DECLARE_FLAG_LOG_FILE;

int
replay(args_t *args, flags_t *flags)
{
    setenv("LOTTO_LOG_FILE", flags_get_sval(flags, FLAG_LOG_FILE), true);

    sys_fprintf(stdout, "trace file: %s\n", flags_get_sval(flags, FLAG_INPUT));

    uint64_t last_clk = cli_trace_last_clk(flags_get_sval(flags, FLAG_INPUT));
    uint64_t goal     = flags_get_uval(flags, FLAG_REPLAY_GOAL);
    if (goal > last_clk) {
        flags_set_default(flags, FLAG_REPLAY_GOAL, uval(last_clk));
    }

    const char *input_fn = flags_get_sval(flags, FLAG_INPUT);
    trace_t *rec         = cli_trace_load(input_fn);
    record_t *first      = trace_next(rec, RECORD_START);
    if (first == NULL) {
        sys_fprintf(stderr, "error: empty trace file '%s'\n", input_fn);
        return 1;
    }

    args = record_args(first);

    preload(flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY),
            flags_is_on(flags, FLAG_VERBOSE),
            !flags_is_on(flags, FLAG_NO_PRELOAD), flags_is_on(flags, FLAG_CREP),
            false, flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    trace_advance(rec);
    statemgr_record_unmarshal(trace_next(rec, RECORD_CONFIG));
    trace_destroy(rec);

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = flags_get_sval(flags, FLAG_INPUT)},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        {NULL}};
    envvar_set(vars, true);

    const char *output_fn = flags_get_sval(flags, FLAG_OUTPUT);
    if (output_fn && output_fn[0]) {
        envvar_t vars[] = {{"LOTTO_RECORD", .sval = output_fn}, {NULL}};
        envvar_set(vars, true);
    }
    record_granularities_t record_granularity =
        flags_get_uval(flags, flag_record_granularity());
    char var[RECORD_GRANULARITIES_MAX_LEN];
    record_granularities_str(record_granularity, var);
    setenv("LOTTO_RECORD_GRANULARITY", var, true);
    if (flags_is_on(flags, FLAG_VERBOSE)) {
        sys_fprintf(stdout, "[lotto] replaying: ");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }
    adjust(flags_get_sval(flags, FLAG_INPUT));
    return execute(args, flags, true);
}

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, FLAG_OUTPUT, sval(""));
    return flags;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_INPUT,
                    FLAG_OUTPUT,
                    FLAG_VERBOSE,
                    FLAG_REPLAY_GOAL,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_BEFORE_RUN,
                    FLAG_AFTER_RUN,
                    FLAG_CREP,
                    FLAG_LOG_FILE,
                    0};
    subcmd_register(replay, "replay", "", "Replay a trace", true, sel,
                    _default_flags, SUBCMD_GROUP_TRACE);
}
