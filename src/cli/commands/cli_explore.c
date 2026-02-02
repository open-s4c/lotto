/*
 */
#include <stdio.h>

#include <lotto/base/envvar.h>
#include <lotto/base/record.h>
#include <lotto/base/tidset.h>
#include <lotto/base/trace.h>
#include <lotto/base/trace_flat.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/cli/args.h>
#include <lotto/cli/exec.h>
#include <lotto/cli/exec_info.h>
#include <lotto/cli/explore.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/memmgr.h>
#include <lotto/cli/flags/sequencer.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/cli/utils.h>
#include <lotto/modules/available/state.h>
#include <lotto/sys/stdio.h>

DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_INPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_LOGGER_BLOCK;
DECLARE_FLAG_LOGGER_FILE;

static uint64_t round_index;
static const category_t cats[] = {CAT_TASK_CREATE, CAT_NONE};

void round_print(flags_t *flags, uint64_t round);

static bool
_cats_has(const category_t cats[], category_t cat)
{
    for (size_t i = 0; cats[i]; i++) {
        if (cats[i] == cat) {
            return true;
        }
    }
    return false;
}

static record_t *
_get_last_record_smaller_or_equal_than_clock(trace_t *recorder, uint64_t clock)
{
    record_t *result;
    for (result = trace_last(recorder);
         result && (result->clk > clock || !(result->kind & RECORD_SCHED) ||
                    result->cat == CAT_NONE || _cats_has(cats, result->cat));
         trace_forget(recorder), result = trace_last(recorder)) {}
    return result;
}

static int
_explore_interval(args_t *args, flags_t *flags, trace_t *input, uint64_t from,
                  uint64_t to, bool expect_failure)
{
    int err      = expect_failure;
    int is_error = expect_failure;
    tidset_t choices;
    tidset_init(&choices);
    record_t *current_record;
    current_record = _get_last_record_smaller_or_equal_than_clock(input, to);
    for (; expect_failure == is_error && from <= to && current_record &&
           current_record->clk >= from;
         trace_forget(input),
         current_record =
             _get_last_record_smaller_or_equal_than_clock(input, to)) {
        statemgr_unmarshal(current_record->data, STATE_TYPE_PERSISTENT, true);
        tidset_t *replay_choices = get_available_tasks();
        if (tidset_size(replay_choices) <=
            tidset_has(replay_choices, current_record->id)) {
            continue;
        }
        tidset_copy(&choices, replay_choices);
        tidset_remove(&choices, current_record->id);
        clk_t clk = current_record->clk;
        for (size_t i = 0;
             ((!expect_failure && !err) || (expect_failure && err)) &&
             i < tidset_size(&choices);
             i++) {
            cli_trace_schedule_task(input, tidset_get(&choices, i));
            cli_trace_save(input, "temp.trace");
            round_print(flags, round_index++);
            is_error = (err = execute(args, flags, false)) != 0;
            if ((expect_failure != is_error) || (expect_failure && err == 130))
                break;
            trace_t *trace = cli_trace_load(flags_get_sval(flags, FLAG_OUTPUT));
            int sub        = _explore_interval(args, flags, trace, clk + 1, to,
                                               expect_failure);
            is_error       = (err = sub) != 0;
            trace_destroy(trace);
        }
    }
    tidset_fini(&choices);
    return err;
}

/**
 * explore
 */
int
explore(args_t *args, flags_t *flags)
{
    preload(flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY),
            flags_is_on(flags, FLAG_VERBOSE),
            !flags_is_on(flags, FLAG_NO_PRELOAD),
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    envvar_t vars[] = {
        {"LOTTO_LOGGER_FILE", .sval = flags_get_sval(flags, FLAG_LOGGER_FILE)},
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, FLAG_OUTPUT)},
        {"LOTTO_REPLAY", .sval = "temp.trace"},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        NULL};
    envvar_set(vars, true);

    if (flags_is_on(flags, FLAG_VERBOSE)) {
        sys_fprintf(stdout, "[lotto] starting:");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }

    trace_t *trace  = cli_trace_load(flags_get_sval(flags, FLAG_INPUT));
    record_t *first = trace_next(trace, RECORD_START);

    args = record_args(first);

    round_index         = 0;
    bool expect_failure = flags_is_on(flags, FLAG_EXPLORE_EXPECT_FAILURE);
    uint64_t min        = flags_get_uval(flags, FLAG_EXPLORE_MIN);
    int err =
        _explore_interval(args, flags, trace, min, UINT64_MAX, expect_failure);
    trace_destroy(trace);
    return err;
}

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_strategy(), sval("random"));
    return flags;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_OUTPUT,
                    FLAG_INPUT,
                    FLAG_VERBOSE,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_EXPLORE_EXPECT_FAILURE,
                    FLAG_EXPLORE_MIN,
                    FLAG_LOGGER_FILE,
                    0};
    subcmd_register(explore, "explore", "", "Exhaustively explore a trace",
                    true, sel, _default_flags, SUBCMD_GROUP_TRACE);
}
