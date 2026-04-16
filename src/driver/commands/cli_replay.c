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
#include <lotto/driver/exec.h>
#include <lotto/driver/exec_info.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/memmgr.h>
#include <lotto/driver/flags/sequencer.h>
#include <lotto/driver/preload.h>
#include <lotto/driver/record.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/trace.h>
#include <lotto/driver/utils.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>

int
replay(args_t *args, flags_t *flags)
{
    uint64_t verbose = flag_verbose_count(flags);

    setenv("LOTTO_LOGGER_FILE", flags_get_sval(flags, flag_logger_file()),
           true);

    sys_fprintf(stdout, "trace file: %s\n",
                flags_get_sval(flags, flag_input()));

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
    args_t replay_args = *args;
    char **dynamic_argv = NULL;
    char **original_argv = replay_args.argv;
    execute_resolve_replay_args(&replay_args, flags);
    if (replay_args.argv != original_argv) {
        dynamic_argv = replay_args.argv;
    }

    preload(flags_get_sval(flags, flag_temporary_directory()), verbose,
            !flags_is_on(flags, flag_no_preload()),
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    trace_advance(rec);
    statemgr_record_unmarshal(trace_next(rec, RECORD_CONFIG));
    trace_destroy(rec);

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = flags_get_sval(flags, flag_input())},
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
    if (verbose > 0) {
        sys_fprintf(stdout, "[lotto] replaying: ");
        args_print(&replay_args);
        sys_fprintf(stdout, "\n");
    }
    adjust(flags_get_sval(flags, flag_input()));
    int ret = execute(&replay_args, flags, true);
    if (dynamic_argv != NULL) {
        sys_free(dynamic_argv);
    }
    return ret;
}

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_output(), sval(""));
    return flags;
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_input(),
                    flag_output(),
                    flag_verbose(),
                    flag_replay_goal(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_before_run(),
                    flag_after_run(),
                    flag_logger_file(),
                    0};
    subcmd_register(replay, "replay", "", "Replay a trace", true, sel,
                    _default_flags, SUBCMD_GROUP_TRACE);
})
