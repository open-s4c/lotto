/*
 */
/*******************************************************************************
 * @file cli_inflex.c
 * @brief Finds the inflection point of a trace.
 *
 * The inflection point hypothesis is that there is one point in the trace after
 * which the program always crash no matter the schedule chosen.
 *
 ******************************************************************************/
#include <assert.h>
#include <limits.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/envvar.h>
#include <lotto/base/record_granularity.h>
#include <lotto/base/trace_flat.h>
#include <lotto/cli/exec.h>
#include <lotto/cli/exec_info.h>
#include <lotto/cli/explore.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/memmgr.h>
#include <lotto/cli/flags/prng.h>
#include <lotto/cli/flags/sequencer.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/replay.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/cli/utils.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stream_file.h>
#include <sys/stat.h>

DECLARE_FLAG_INPUT;
DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_ROUNDS;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_LOGGER_BLOCK;
DECLARE_FLAG_BEFORE_RUN;
DECLARE_FLAG_AFTER_RUN;
DECLARE_FLAG_LOGGER_FILE;
DECLARE_COMMAND_FLAG(INFLEX_MIN, "", "inflex-min", "UINT",
                     "minimum clock for the inflection point", flag_uval(0))
DECLARE_COMMAND_FLAG(INFLEX_METHOD, "", "inflex-method", "METHOD",
                     "b(inary) / l(inear) search + p(robablistic)/e(xplore)",
                     flag_sval("bp"))

DECLARE_FLAG_REPLAY_GOAL;

void round_print(flags_t *flags, uint64_t round);

static bool _fails(const args_t *args, flags_t *flags);
static clk_t _truncate_trace(const char *input, const char *output, clk_t clk);
static bool _produce_full_trace_and_set_input(args_t *args, flags_t *flags);
static clk_t _previous_clock(const char *input, clk_t clk);
static clk_t _next_clock(const char *input, clk_t clk);

static int _binary_probablistic(args_t *args, flags_t *flags,
                                uint64_t last_clk);
static int _binary_explore(args_t *args, flags_t *flags, uint64_t last_clk);

typedef bool (*predicate_t)(args_t *, flags_t *, clk_t);
typedef clk_t (*advance_clock_t)(const char *, clk_t);
static int _linear_inflex(args_t *args, flags_t *flags, uint64_t last_clk,
                          predicate_t pred, bool use_explore);
static clk_t _binary_search(args_t *args, flags_t *flags, clk_t l, clk_t r,
                            predicate_t pred);
static void _linear_search(args_t *args, flags_t *flags, clk_t *beg, clk_t *end,
                           predicate_t pred, advance_clock_t advance);
static bool _fails_at_clk(args_t *args, flags_t *flags, clk_t clk);
static bool _always_fails_at_clk_prob(args_t *args, flags_t *flags, clk_t clk);
static bool _always_fails_at_clk_explore(args_t *args, flags_t *flags,
                                         clk_t clk);

/**
 * inflection point finder
 */
int
inflex(args_t *args, flags_t *flags)
{
    setenv("LOTTO_LOGGER_FILE", flags_get_sval(flags, FLAG_LOGGER_FILE), true);

    sys_fprintf(stdout, "input trace: %s\n", flags_get_sval(flags, FLAG_INPUT));
    sys_fprintf(stdout, "output trace: %s\n",
                flags_get_sval(flags, FLAG_OUTPUT));

    trace_t *rec = cli_trace_load(flags_get_sval(flags, FLAG_INPUT));
    ASSERT(rec);
    record_t *first = trace_next(rec, RECORD_START);
    record_t *last  = trace_last(rec);

    args = record_args(first);

    uint64_t k = last->clk;
    trace_destroy(rec);

    if (flags_is_on(flags, FLAG_VERBOSE)) {
        sys_fprintf(stdout, "[lotto] inflex: ");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }

    preload(flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY),
            flags_is_on(flags, FLAG_VERBOSE),
            !flags_is_on(flags, FLAG_NO_PRELOAD), false,
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    const char *method = flags_get_sval(flags, FLAG_INFLEX_METHOD);
    bool use_linear    = strchr(method, 'l') != NULL;
    bool use_explore   = strchr(method, 'e') != NULL;

    if (use_linear) {
        sys_fprintf(stdout, "[lotto] inflex: use linear search\n");
    }
    if (use_explore) {
        sys_fprintf(stdout, "[lotto] inflex: use explore\n");
    }

    if (!use_linear && !use_explore) {
        return _binary_probablistic(args, flags, k);
    } else if (use_linear && !use_explore) {
        return _linear_inflex(args, flags, k, _always_fails_at_clk_prob,
                              use_explore);
    } else if (!use_linear && use_explore) {
        return _binary_explore(args, flags, k);
    } else if (use_linear && use_explore) {
        return _linear_inflex(args, flags, k, _always_fails_at_clk_explore,
                              use_explore);
    }

    ASSERT(false && "unreachable");
    return 0;
}

static int
_binary_probablistic(args_t *args, flags_t *flags, uint64_t last_clk)
{
    /* force output to be temporarily another */
    const char *output = flags_get_sval(flags, FLAG_OUTPUT);

    char filename[PATH_MAX];
    sys_sprintf(filename, "%s/temp.trace",
                flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY));
    flags_set_default(flags, FLAG_OUTPUT, sval(filename));

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = flags_get_sval(flags, FLAG_INPUT)},
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, FLAG_OUTPUT)},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        {"LOTTO_LOGGER_LEVEL", .sval = "silent"},
        {NULL}};
    envvar_set(vars, true);

    char candidate_filename[PATH_MAX];
    sys_sprintf(candidate_filename, "%s/candidate.trace",
                flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY));

    uint64_t ip         = flags_get_uval(flags, FLAG_INFLEX_MIN);
    uint64_t last_ip    = UINT64_MAX;
    uint64_t confidence = 0;
    while (confidence < flags_get_uval(flags, FLAG_ROUNDS) && ip < last_clk) {
        sys_fprintf(stdout, "[lotto] inflex: computing confidence %lu\n",
                    confidence);
        ip = _binary_search(args, flags, ip, last_clk, _fails_at_clk);
        if (ip == last_ip) {
            confidence++;
            continue;
        }
        last_ip    = ip;
        confidence = 1;
        sys_fprintf(
            stdout,
            "[lotto] inflex: updating the inflex candidate %s for clock %lu, "
            "confidence is reset to 1\n",
            candidate_filename, ip);
        /* save the candidate */
        _truncate_trace(flags_get_sval(flags, FLAG_INPUT), candidate_filename,
                        ip);
    }

    sys_fprintf(stdout, "[lotto] inflection point = %lu\n", ip);
    trace_t *rec = cli_trace_load(candidate_filename);
    cli_trace_save(rec, output);
    trace_destroy(rec);

    return 0;
}

static int
_binary_explore(args_t *args, flags_t *flags, uint64_t last_clk)
{
    const char *output = flags_get_sval(flags, FLAG_OUTPUT);
    char filename[PATH_MAX];
    sys_sprintf(filename, "%s/temp.trace",
                flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY));
    flags_set_default(flags, FLAG_OUTPUT, sval(filename));

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = flags_get_sval(flags, FLAG_INPUT)},
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, FLAG_OUTPUT)},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        {"LOTTO_LOGGER_LEVEL", .sval = "silent"},
        {NULL}};
    envvar_set(vars, true);

    flags_set_by_opt(flags, FLAG_EXPLORE_EXPECT_FAILURE, on());

    /* Explore the minimal trace to locate the range. */
    uint64_t min = flags_get_uval(flags, FLAG_INFLEX_MIN);
    uint64_t max = last_clk;
    max          = _binary_search(args, flags, min, max, _fails_at_clk);

    /* For minimal traces (those that do not record every chpt),
     * explore can only explore a subset of the state space
     * (underestimation), which means the previous search does not
     * find the real inflection point.
     *
     * To find the real inflection point, the range must be relaxed
     * due to how explore works: max should be adjusted to the
     * smallest recorded clock greater than max.
     *
     * Then, proceed to explore exhaustively by obtaining a full trace
     * first.
     */
    max = _next_clock(flags_get_sval(flags, FLAG_INPUT), max);
    sys_fprintf(stdout,
                "[lotto] real inflection point is between %lu and %lu\n", min,
                max);

    if (!_produce_full_trace_and_set_input(args, flags)) {
        sys_fprintf(stderr, "[lotto] couldn't produce a full trace!\n");
        return 1;
    }

    max = _binary_search(args, flags, min, max, _always_fails_at_clk_explore);
    sys_fprintf(stdout, "[lotto] inflection point = %lu\n", max);
    _truncate_trace(flags_get_sval(flags, FLAG_INPUT), output, max);
    return 0;
}

static int
_linear_inflex(args_t *args, flags_t *flags, uint64_t last_clk,
               predicate_t pred, bool use_explore)
{
    const char *output = flags_get_sval(flags, FLAG_OUTPUT);
    char filename[PATH_MAX];
    sys_sprintf(filename, "%s/temp.trace",
                flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY));
    flags_set_default(flags, FLAG_OUTPUT, sval(filename));

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = flags_get_sval(flags, FLAG_INPUT)},
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, FLAG_OUTPUT)},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        {"LOTTO_LOGGER_LEVEL", .sval = "silent"},
        {NULL}};
    envvar_set(vars, true);

    /* beg = the clock of the record where some runs can succeed
     * end = the clock of the record where all runs fail
     * the inflection point must be within (beg, end]
     */
    clk_t beg = 0, end = last_clk;
    _linear_search(args, flags, &beg, &end, pred, _previous_clock);

    if (use_explore) {
        end      = _next_clock(flags_get_sval(flags, FLAG_INPUT), end);
        bool ret = _produce_full_trace_and_set_input(args, flags);
        if (!ret) {
            sys_fprintf(stderr,
                        "[lotto] couldn't produce the full trace file!\n");
            return 1;
        }
    }

    sys_fprintf(stdout,
                "[lotto] real inflection point is between %lu and %lu\n", beg,
                end);
    ASSERT(end >= beg && "clock range is incorrect");

    clk_t ip = end > 0 ? end - 1 : 0;
    _linear_search(args, flags, &beg, &ip, pred, NULL);
    sys_fprintf(stdout, "[lotto] inflection point = %lu\n", ip);
    _truncate_trace(flags_get_sval(flags, FLAG_INPUT), output, ip);
    return 0;
}

/* Find the smallest clk where pred is true (assuming pred is monotone) */
static void
_linear_search(args_t *args, flags_t *flags, clk_t *beg, clk_t *end,
               predicate_t pred, advance_clock_t advance)
{
    const char *input = flags_get_sval(flags, FLAG_INPUT);
    for (clk_t clk = *end; clk > *beg;
         advance ? (clk = advance(input, clk)) : --clk) {
        sys_fprintf(stdout, "[lotto] inflex: probing clock %lu\n", clk);
        if (pred(args, flags, clk)) {
            *end = clk;
        } else {
            *beg = clk;
            break;
        }
    }
}

/* Find the smallest clk where pred is true (assuming pred is monotone) */
static clk_t
_binary_search(args_t *args, flags_t *flags, clk_t l, clk_t r, predicate_t pred)
{
    while (r > l) {
        clk_t x = l + (r - l) / 2;
        if (pred(args, flags, x)) {
            r = x;
        } else {
            l = x + 1;
        }
    }
    return r;
}

static bool
_fails(const args_t *args, flags_t *flags)
{
    struct value val = uval(now());
    flags_set_by_opt(flags, flag_seed(), val);

    int return_code = execute(args, flags, true);
    if (return_code == 130) {
        /* exit on CTRL-C */
        exit(130);
    }
    adjust(flags_get_sval(flags, FLAG_OUTPUT));
    if (return_code == 0) {
        return false;
    }
    return true;
}

static bool
_fails_at_clk(args_t *args, flags_t *flags, clk_t clk)
{
    flags_set_by_opt(flags, FLAG_REPLAY_GOAL, uval(clk));
    bool fail = _fails(args, flags);
    return fail;
}

static bool
_always_fails_at_clk_prob(args_t *args, flags_t *flags, clk_t clk)
{
    uint64_t rounds   = flags_get_uval(flags, FLAG_ROUNDS);
    size_t confidence = 1;
    bool always_fail  = true;
    while (confidence <= rounds) {
        sys_fprintf(stdout, "\r        confidence=%lu/%lu", confidence, rounds);
        if (!_fails_at_clk(args, flags, clk)) {
            always_fail = false;
            break;
        } else {
            confidence++;
        }
    }
    sys_fprintf(stdout, "\n");
    return always_fail;
}

static bool
_always_fails_at_clk_explore(args_t *args, flags_t *flags, clk_t clk)
{
    flags_set_by_opt(flags, FLAG_EXPLORE_EXPECT_FAILURE, on());
    flags_set_by_opt(flags, FLAG_EXPLORE_MIN, uval(clk + 1));
    int err = explore(args, flags);
    /* err!=0 means always fails when expect-failure is set */
    return err;
}

static bool
_produce_full_trace_and_set_input(args_t *args, flags_t *flags)
{
    static char full_trace_path[PATH_MAX];
    const char *dir = flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY);
    sys_sprintf(full_trace_path, "%s/full.trace", dir);

    flags_t *replay_flags = flagmgr_flags_alloc();
    flags_cpy(replay_flags, flags_default());
    flags_set_by_opt(replay_flags, flag_record_granularity(),
                     uval(RECORD_GRANULARITY_CHPT));
    flags_set_by_opt(replay_flags, FLAG_INPUT,
                     sval(flags_get_sval(flags, FLAG_INPUT)));
    flags_set_by_opt(replay_flags, FLAG_OUTPUT, sval(full_trace_path));
    flags_set_by_opt(replay_flags, FLAG_NO_PRELOAD, on());
    flags_set_by_opt(replay_flags, FLAG_TEMPORARY_DIRECTORY, sval(dir));
    replay(args, replay_flags);
    flags_free(replay_flags);

    struct stat stbuf;
    if (stat(full_trace_path, &stbuf)) {
        return false;
    }
    flags_set_by_opt(flags, FLAG_INPUT, sval(full_trace_path));
    return true;
}

static clk_t
_truncate_trace(const char *input, const char *output,
                clk_t target /* inclusive */)
{
    trace_t *rec   = cli_trace_load(input);
    record_t *r    = NULL;
    clk_t last_clk = 0;
    while ((r = trace_last(rec)) && r->clk > target) {
        trace_forget(rec);
    }
    if (r && r->clk < target) {
        last_clk = r->clk;
        r        = record_alloc(0);
        r->kind  = RECORD_OPAQUE;
        r->clk   = target;
        trace_append(rec, r);
    }
    cli_trace_save(rec, output);
    trace_destroy(rec);
    return last_clk;
}

static clk_t
_next_clock(const char *input, clk_t clk)
{
    trace_t *rec = cli_trace_load(input);
    clk_t ans    = clk;
    for (record_t *r; (r = trace_last(rec)) && r->clk > ans; trace_forget(rec))
        ans = r->clk;
    trace_destroy(rec);
    return ans;
}

static clk_t
_previous_clock(const char *input, clk_t clk)
{
    trace_t *rec = cli_trace_load(input);
    trace_next(rec, RECORD_START);
    clk_t ans = clk;
    for (record_t *r; (r = trace_next(rec, RECORD_ANY)) && r->clk < clk;
         trace_advance(rec))
        ans = r->clk;
    trace_destroy(rec);
    return ans;
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
                    FLAG_ROUNDS,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_BEFORE_RUN,
                    FLAG_AFTER_RUN,
                    FLAG_INFLEX_MIN,
                    FLAG_INFLEX_METHOD,
                    FLAG_LOGGER_FILE,
                    0};
    subcmd_register(inflex, "inflex", "", "Find an inflection point of a trace",
                    true, sel, _default_flags, SUBCMD_GROUP_TRACE);
}
