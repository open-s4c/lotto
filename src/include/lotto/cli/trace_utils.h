/*
 */
#ifndef LOTTO_CLI_TRACE_UTILS_H
#define LOTTO_CLI_TRACE_UTILS_H

#include <lotto/base/flags.h>
#include <lotto/base/tidset.h>
#include <lotto/base/trace.h>
#include <lotto/cli/args.h>

trace_t *cli_trace_load(const char *fn);
void cli_trace_save(const trace_t *rec, const char *fn);
uint64_t cli_trace_last_clk(const char *name);
void cli_trace_trim_to_clk(trace_t *trace, clk_t clk);
void cli_trace_trim_to_goal(trace_t *trace, clk_t goal, bool drop_old_config);
void cli_trace_trim_to_cat(trace_t *trace, category_t cat);
void cli_trace_trim_to_kind(trace_t *trace, kinds_t kinds);
tidset_t *cli_trace_alternative_tasks(trace_t *trace);
void cli_trace_schedule_task(trace_t *trace, task_id task);
void cli_trace_check_hash(trace_t *t, uint64_t hash);
void cli_trace_copy(const char *from, const char *to);
const char *detect_record_type(const char *fn);

#define MAX_GOAL (~(clk_t)0)

/**
 * Prepares traces for the run. The output trace with the start record is
 * created. If config is set and the output trace, its trimmed copy suffixed
 * with ".tmp" is created. The copy is appended with a config record.
 * LOTTO_REPLAY is set to this copy.
 *
 * @param record     output trace name, NULL is equivalent to "replay.trace"
 * @param args       argument object
 * @param replay     input trace name
 * @param fgoal      replay goal flag
 * @param config     whether the config record should be inserted into the
 * replay
 * @param flags      the cli flags to be published to the subscribers
 * trace
 */
void cli_trace_init(const char *record, const args_t *args, const char *replay,
                    struct flag_val fgoal, bool config, const flags_t *flags);

record_t *record_config(clk_t goal);

/**
 * Creates a starting record with args and config
 *
 * @param args       CLI args of the target program
 */
record_t *record_start(const args_t *args);

/**
 * Prints record information
 *
 * @param r const record pointer
 * @param i const record index
 */
void record_print(const record_t *r, int i);

args_t *record_args(const record_t *r);

#endif
