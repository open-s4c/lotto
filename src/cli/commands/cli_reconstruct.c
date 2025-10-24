/*
 */
#include <limits.h>

#include <lotto/base/envvar.h>
#include <lotto/base/log.h>
#include <lotto/base/record_granularity.h>
#include <lotto/base/tidset.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/cli/exec.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/memmgr.h>
#include <lotto/cli/flags/prng.h>
#include <lotto/cli/flags/sequencer.h>
#include <lotto/cli/log_utils.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/cli/utils.h>
#include <lotto/states/handlers/reconstruct.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

#define SEL_SCHED         (RECORD_START | RECORD_SCHED | RECORD_FORCE)
#define TEMP_INPUT_TRACE  "temp_i.trace"
#define TEMP_OUTPUT_TRACE "temp_o.trace"

DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_ROUNDS;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_LOGGER_BLOCK;
DECLARE_FLAG_CREP;
DECLARE_FLAG_LOG_FILE;
DECLARE_COMMAND_FLAG(RECONSTRUCT_LOG, "", "reconstruct-log", "FILE",
                     "reconstruction log FILE", flag_sval("reconstruct.log"))
DECLARE_COMMAND_FLAG(RECONSTRUCT_STRATEGY, "", "reconstruct-strategy", "STRAT",
                     "reconstruction strategy ('explore' | 'random')",
                     flag_sval("explore"))
DECLARE_COMMAND_FLAG(RECONSTRUCT_ROUNDS, "", "reconstruct-rounds", "INT",
                     "reconstruction rounds for random search", flag_uval(20))
DECLARE_COMMAND_FLAG(RECONSTRUCT_NO_EQUIVALENCE_CHECK, "",
                     "reconstruct-no-equivalence-check", "",
                     "reconstruction no equivalence check"
                     "(has effect only for strategy 'explore')",
                     flag_off())
DECLARE_COMMAND_FLAG(RECONSTRUCT_NO_VIOLATION_SEARCH, "",
                     "reconstruct-no-violation-search", "",
                     "reconstruction no violation search", flag_off())
DECLARE_COMMAND_FLAG(RECONSTUCT_NONATOMIC, "", "reconstruct-nonatomic", "",
                     "reconstruction nonatomic", flag_off())

DECLARE_FLAG_REPLAY_GOAL;

typedef struct conf_s {
    const char *output;
    const uint64_t rounds;
    const uint64_t reconstruct_rounds;
    const bool no_equivalence_check;
    const bool no_violation_search;
} conf_t;

static args_t *args;
static flags_t *flags;
static conf_t *conf;
static log_t *log;
static trace_t *trace;
static log_t *entry;
static int idx;

static int _explore_prefix(int idx_max, clk_t clk_min);
static int _explore_suffix(int idx_max);

char temp_input_trace[PATH_MAX];
char temp_output_trace[PATH_MAX];

conf_t *
config_create(const flags_t *flags)
{
    const char *output = flags_get_sval(flags, FLAG_OUTPUT);

    uint64_t rounds = flags_get_uval(flags, FLAG_ROUNDS);

    uint64_t reconstruct_rounds =
        flags_get_uval(flags, FLAG_RECONSTRUCT_ROUNDS);

    uint64_t no_equivalence_check =
        flags_is_on(flags, FLAG_RECONSTRUCT_NO_EQUIVALENCE_CHECK);

    uint64_t no_violation_search =
        flags_is_on(flags, FLAG_RECONSTRUCT_NO_VIOLATION_SEARCH);

    ASSERT(reconstruct_rounds > 0 &&
           "Reconstruction rounds must be greater than zero");

    if (reconstruct_rounds > rounds) {
        rounds = reconstruct_rounds;
    }

    conf_t c_struct = (conf_t){output, rounds, reconstruct_rounds,
                               no_equivalence_check, no_violation_search};
    conf_t *c_ptr   = (conf_t *)sys_malloc(sizeof(conf_t));
    sys_memcpy(c_ptr, &c_struct, sizeof(conf_t));
    return c_ptr;
}

static void
_config_init()
{
    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = temp_input_trace},
        {"LOTTO_RECORD", .sval = temp_output_trace},
        {"LOTTO_LOG_FILE", .sval = flags_get_sval(flags, FLAG_LOG_FILE)},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        NULL};
    envvar_set(vars, true);

    flags_set_by_opt(flags, FLAG_OUTPUT, sval(temp_output_trace));
}

static void
_trace_trim_schedule_point()
{
    cli_trace_trim_to_kind(trace, SEL_SCHED);
    if (trace_last(trace) != NULL) {
        if (trace_last(trace)->cat == CAT_LOG_AFTER) {
            idx--;
        }
        trace_forget(trace);
        cli_trace_trim_to_kind(trace, SEL_SCHED);
    }
    ASSERT(trace_last(trace) != NULL);
}

static void
_trace_trim_to_idx(int idx_new)
{
    ASSERT(idx > 0 && idx >= idx_new);
    while (idx > idx_new) {
        _trace_trim_schedule_point();
        ASSERT(trace_last(trace) != NULL);
    }
    cli_trace_trim_to_cat(trace, CAT_LOG_AFTER);
}

static category_t _cats[] = {
    CAT_BEFORE_AWRITE, CAT_BEFORE_AREAD,   CAT_BEFORE_XCHG,
    CAT_BEFORE_RMW,    CAT_BEFORE_CMPXCHG, CAT_BEFORE_FENCE,
    CAT_SYS_YIELD,     CAT_USER_YIELD,     CAT_NONE};

static bool
_cats_has(category_t cat)
{
    if (flags_is_on(flags, FLAG_RECONSTUCT_NONATOMIC) &&
        (cat == CAT_BEFORE_READ || cat == CAT_BEFORE_WRITE)) {
        return true;
    }
    for (size_t i = 0; _cats[i]; i++) {
        if (_cats[i] == cat) {
            return true;
        }
    }
    return false;
}

static bool
_trace_trim_to_chpt(const clk_t clk_min, const task_id tid)
{
    record_t *record = trace_last(trace);
    category_t cat   = record->cat;

    while (record->clk > clk_min) {
        if (_cats_has(cat) && (tid == NO_TASK || record->id == tid)) {
            return true;
        }
        _trace_trim_schedule_point();

        record = trace_last(trace);
        cat    = record->cat;
    }

    ASSERT(record->clk >= clk_min);
    return _cats_has(cat);
}

static void
_trace_trim_exit_and_update_log(const clk_t clk_min)
{
    record_t *record = trace_last(trace);
    ASSERT(record->kind & RECORD_EXIT);
    if (record->reason == REASON_ASSERT_FAIL)
        return;
    cli_trace_trim_to_kind(trace, SEL_SCHED);
    record = trace_last(trace);
    if (record->clk > clk_min) {
        if (record->cat == CAT_LOG_AFTER) {
            statemgr_unmarshal(record->data, STATE_TYPE_PERSISTENT, false);
            log_t *log_ptr = reconstruct_get_log();
            sys_memcpy(entry, log_ptr, sizeof(log_t));
        }
    } else {
        ASSERT(record->clk == clk_min && record->cat != CAT_LOG_AFTER);
    }
}

static clk_t
_get_equivalence_trim_clk(clk_t clk_min, task_id task)
{
    trace_t *trace_copy = cli_trace_load(temp_output_trace);

    clk_t clk        = 0;
    record_t *record = trace_last(trace_copy);
    category_t cat   = record->cat;

    while (record != NULL && record->clk >= clk_min) {
        if (_cats_has(cat)) {
            if (record->id == task) {
                clk = record->clk;
            } else {
                clk = 0;
            }
        }
        trace_forget(trace_copy);

        record = trace_last(trace_copy);
        cat    = record->cat;
    }
    trace_destroy(trace_copy);
    return clk;
}

static int
_run_step(const uint64_t goal)
{
    flags_set_by_opt(flags, flag_seed(), uval(now()));

    reconstruct_config()->enabled = true;
    reconstruct_config()->tid     = log[idx].tid;
    reconstruct_config()->id      = log[idx].id;
    reconstruct_config()->data    = log[idx].data;

    cli_trace_trim_to_goal(trace, goal, true);
    record_t *record = record_config(goal);
    if (TRACE_OK != trace_append(trace, record))
        ASSERT(0 && "could not append");
    cli_trace_save(trace, temp_input_trace);

    int err = execute(args, flags, false);
    if (adjust(flags_get_sval(flags, FLAG_OUTPUT))) {
        sys_fprintf(stdout, "[lotto] internal error occurred, terminating\n");
        exit(1);
    }
    trace_destroy(trace);
    trace = cli_trace_load(temp_output_trace);
    return err;
}

static int
_random_search(const int idx_max, const bool explore)
{
    int err = 0;

    const clk_t clk_min_orig = trace_last(trace)->clk;
    clk_t clk_min            = clk_min_orig;
    uint64_t round           = 0;

    while (idx < idx_max && round < conf->reconstruct_rounds) {
        err        = _run_step(clk_min);
        entry->tid = NO_TASK;
        round++;

        if (log[idx].tid == NO_TASK) {
            ASSERT(idx == idx_max - 1);
            if (conf->no_violation_search ||
                (err && trace_last(trace)->reason == REASON_ASSERT_FAIL)) {
                idx = idx_max;
                break;
            }
        }

        _trace_trim_exit_and_update_log(clk_min);

        if (entry->tid != NO_TASK) {
            if (log_equals(entry, log + idx)) {
                idx++;
                round   = 0;
                clk_min = trace_last(trace)->clk;
                continue;
            }
            cli_trace_trim_to_cat(trace, CAT_LOG_BEFORE);
            if (explore && log[idx].tid == entry->tid) {
                break;
            }
        }

        if (explore && trace_last(trace)->clk == clk_min_orig) {
            break;
        }
    }
    return err;
}

static int
_explore_suffix(const int idx_max)
{
    int err = 0;

    const int idx_min   = idx;
    const clk_t clk_min = trace_last(trace)->clk;
    task_id task_orig   = trace_last(trace)->id;
    tidset_t *tidset    = cli_trace_alternative_tasks(trace);

    if (tidset != NULL) {
        const size_t count = tidset_size(tidset);
        for (size_t i = 0; i < count && idx < idx_max; i++) {
            ASSERT(idx == idx_min && trace_last(trace)->clk == clk_min);
            cli_trace_schedule_task(trace, tidset_get(tidset, i));
            err = _random_search(idx_max, true);
            if (idx >= idx_max || trace_last(trace)->clk <= clk_min) {
                continue;
            }

            if (!conf->no_equivalence_check) {
                clk_t clk_trim = _get_equivalence_trim_clk(clk_min, task_orig);
                if (clk_trim > clk_min) {
                    while (trace_last(trace)->clk > clk_trim) {
                        _trace_trim_schedule_point();
                    }
                    ASSERT(trace_last(trace)->clk == clk_trim);
                    _explore_suffix(idx_max);
                    if (idx < idx_max) {
                        while (trace_last(trace)->clk > clk_min) {
                            _trace_trim_schedule_point();
                        }
                    }
                    continue;
                }
            }

            err = _explore_prefix(idx_max, clk_min + 1);
            if (idx >= idx_max || trace_last(trace)->clk <= clk_min) {
                continue;
            }
            _trace_trim_schedule_point();
        }
        tidset_fini(tidset);
    }

    ASSERT(idx == idx_max ||
           (idx == idx_min && trace_last(trace)->clk == clk_min));
    return err;
}

static int
_explore_prefix(const int idx_max, const clk_t clk_min)
{
    int err = 0;
    ASSERT(trace_last(trace) != NULL && trace_last(trace)->clk >= clk_min);

    // Special handling for mismatching log entry
    if (entry->tid != NO_TASK) {
        if (entry->tid != log[idx].tid) {
            err = _explore_suffix(idx_max);
            if (idx == idx_max || trace_last(trace)->clk == clk_min) {
                return err;
            }
            _trace_trim_schedule_point();
        } else {
            _trace_trim_to_chpt(clk_min, entry->tid);
        }
    }

    // Normal exploration of trace prefix
    while (!(trace_last(trace)->kind & RECORD_START)) {
        if (_trace_trim_to_chpt(clk_min, NO_TASK)) {
            err = _explore_suffix(idx_max);
        }
        if (idx == idx_max || trace_last(trace)->clk == clk_min) {
            return err;
        }
        _trace_trim_schedule_point();
    }

    ASSERT(trace_last(trace)->clk == clk_min);
    return err;
}

static int
_run_reconstruct_explore(const int size)
{
    int err;
    while (idx < size) {
        err = _random_search(size, false);
        if (idx < size) {
            err = _explore_prefix(idx + 1, 0);
            if (idx == 0) {
                return 0;
            }
        }
    }
    return err;
}

static int
_run_reconstruct_random(const int size)
{
    int err;
    int idx_min     = size;
    int idx_max     = -1;
    uint64_t rounds = 0;

    while (true) {
        err = _random_search(size, false);
        if (idx == size) {
            return err;
        }

        rounds += idx > 0 ? 1 : conf->reconstruct_rounds;

        if (idx > idx_max) {
            idx_max = idx;
            if (idx > 0) {
                idx_min = idx - 1;
                rounds  = 0;
            } else {
                idx_min = 0;
            }
        }
        if (idx_min > 0) {
            if (rounds >= conf->reconstruct_rounds) {
                idx_min--;
                rounds = 0;
            }
            if (idx_min > 0) {
                _trace_trim_to_idx(idx_min);
            }
        }
        if (idx_min == 0) {
            idx = 0;
            if (rounds >= conf->rounds) {
                return 0;
            }
            cli_trace_trim_to_clk(trace, 0);
        }
    }
}

static int
_run_reconstruct(const int size)
{
    const char *strategy = flags_get_sval(flags, FLAG_RECONSTRUCT_STRATEGY);
    ASSERT(strategy);

    if (!strcmp(strategy, "explore"))
        return _run_reconstruct_explore(size);

    if (!strcmp(strategy, "random"))
        return _run_reconstruct_random(size);

    ASSERT(0 && "Illegal reconstruction strategy");
    return -1;
}

int
reconstruct(args_t *a, flags_t *f)
{
    conf = config_create(f);

    preload(flags_get_sval(f, FLAG_TEMPORARY_DIRECTORY),
            flags_is_on(f, FLAG_VERBOSE), !flags_is_on(f, FLAG_NO_PRELOAD),
            flags_is_on(f, FLAG_CREP), false,
            flags_get_sval(f, flag_memmgr_runtime()),
            flags_get_sval(f, flag_memmgr_user()));

    sys_sprintf(temp_input_trace, "%s/%s",
                flags_get_sval(f, FLAG_TEMPORARY_DIRECTORY), TEMP_INPUT_TRACE);
    sys_sprintf(temp_output_trace, "%s/%s",
                flags_get_sval(f, FLAG_TEMPORARY_DIRECTORY), TEMP_OUTPUT_TRACE);

    flags = f;
    args  = a;
    _config_init();

    int size;
    log   = log_load(flags_get_sval(flags, FLAG_RECONSTRUCT_LOG), &size);
    entry = sys_malloc(sizeof(log_t));
    log_init(entry);
    cli_trace_init(temp_input_trace, args, NULL,
                   flags_get(flags, FLAG_REPLAY_GOAL), false, flags);
    trace = cli_trace_load(getenv("LOTTO_REPLAY"));
    _config_init();

    int err = _run_reconstruct(size);

    if (idx == size) {
        sys_fprintf(stdout, "[lotto] Trace reconstructed successfully!\n");
        cli_trace_save(trace, conf->output);
    } else if (idx == 0) {
        sys_fprintf(stdout, "[lotto] Trace reconstruction failed.\n");
        err = 0;
    } else {
        ASSERT(false && "Invalid reconstruction result");
    }

    trace_destroy(trace);
    sys_free(entry);
    sys_free(log);
    sys_free(conf);

    return err;
}

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_strategy(), sval("random"));
    flags_set_default(flags, flag_record_granularity(),
                      uval(RECORD_GRANULARITY_CAPTURE));
    return flags;
}

static void LOTTO_CONSTRUCTOR
_init()
{
    flag_t sel[] = {FLAG_OUTPUT,
                    FLAG_VERBOSE,
                    FLAG_ROUNDS,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_RECONSTRUCT_LOG,
                    FLAG_RECONSTRUCT_STRATEGY,
                    FLAG_RECONSTRUCT_ROUNDS,
                    FLAG_RECONSTRUCT_NO_EQUIVALENCE_CHECK,
                    FLAG_RECONSTRUCT_NO_VIOLATION_SEARCH,
                    FLAG_RECONSTUCT_NONATOMIC,
                    FLAG_CREP,
                    FLAG_LOG_FILE,
                    0};
    subcmd_register(reconstruct, "reconstruct", "[--] <command line>",
                    "Reconstruct execution trace of a program", true, sel,
                    _default_flags, SUBCMD_GROUP_RUN);
}
