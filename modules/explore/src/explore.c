#include <stdio.h>

#include <lotto/base/envvar.h>
#include <lotto/base/record.h>
#include <lotto/base/tidset.h>
#include <lotto/base/trace.h>
#include <lotto/base/trace_flat.h>
#include <lotto/driver/args.h>
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
#include <lotto/engine/statemgr.h>
#include <lotto/modules/available/state.h>
#include <lotto/modules/explore/explore.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/stdio.h>

static uint64_t round_index;
static const type_id ignored_types[] = {EVENT_TASK_CREATE, 0};

void round_print(flags_t *flags, uint64_t round);

static bool
_types_has(const type_id types[], type_id type)
{
    for (size_t i = 0; types[i]; i++) {
        if (types[i] == type) {
            return true;
        }
    }
    return false;
}

static inline type_id
_record_effective_type(const record_t *r)
{
    return r->type_id;
}

static record_t *
_get_last_record_smaller_or_equal_than_clock(trace_t *recorder, uint64_t clock)
{
    record_t *result;
    for (result = trace_last(recorder);
         result && (result->clk > clock || !(result->kind & RECORD_SCHED) ||
                    _record_effective_type(result) == 0 ||
                    _types_has(ignored_types, _record_effective_type(result)));
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
            trace_t *trace =
                cli_trace_load(flags_get_sval(flags, flag_output()));
            int sub  = _explore_interval(args, flags, trace, clk + 1, to,
                                         expect_failure);
            is_error = (err = sub) != 0;
            trace_destroy(trace);
        }
    }
    tidset_fini(&choices);
    return err;
}

int
explore(args_t *args, flags_t *flags)
{
    uint64_t verbose = flag_verbose_count(flags);

    preload(flags_get_sval(flags, flag_temporary_directory()), verbose,
            !flags_is_on(flags, flag_no_preload()),
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    envvar_t vars[] = {
        {"LOTTO_LOGGER_FILE",
         .sval = flags_get_sval(flags, flag_logger_file())},
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, flag_output())},
        {"LOTTO_REPLAY", .sval = "temp.trace"},
        NULL};
    envvar_set(vars, true);

    if (verbose > 0) {
        sys_fprintf(stdout, "[lotto] starting:");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }

    trace_t *trace  = cli_trace_load(flags_get_sval(flags, flag_input()));
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
