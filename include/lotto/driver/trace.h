#ifndef LOTTO_DRIVER_TRACE_H
#define LOTTO_DRIVER_TRACE_H

#include <lotto/base/tidset.h>
#include <lotto/base/trace.h>

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

#endif
