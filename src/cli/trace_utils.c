/*
 */
#include <dirent.h>
#include <inttypes.h>
#include <libgen.h>
#include <stdint.h>

#include <lotto/base/envvar.h>
#include <lotto/base/flag.h>
#include <lotto/base/record.h>
#include <lotto/base/trace_chunked.h>
#include <lotto/base/trace_file.h>
#include <lotto/base/trace_flat.h>
#include <lotto/brokers/catmgr.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/cli/exec_info.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/states/handlers/available.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream_chunked_file.h>
#include <lotto/sys/stream_file.h>
#include <lotto/sys/string.h>
#include <sys/stat.h>

record_t *
record_config(const clk_t goal)
{
    // create CONFIG record using the statemgr. The flag values are by now
    // received by the subscribers and their configurations are ready to be
    // marshaled.
    record_t *r = record_alloc(statemgr_size(STATE_TYPE_CONFIG));
    r->kind     = RECORD_CONFIG;
    r->clk      = goal;
    (void)statemgr_marshal(r->data, STATE_TYPE_CONFIG);
    return r;
}

trace_t *
cli_trace_load(const char *fn)
{
    char *var = sys_getenv("LOTTO_RECORDER_TYPE");

    if (!var) {
        const char *real_fn = detect_record_type(fn);
        fn                  = real_fn;
        var                 = sys_getenv("LOTTO_RECORDER_TYPE");
        ASSERT(var);
    }

    trace_t *rec = NULL;
    if (sys_strcmp(var, "flat") == 0) {
        stream_t *st = stream_file_alloc();
        stream_file_in(st, fn);
        rec = trace_flat_create(st);
        ASSERT(rec);
        trace_load(rec);
        stream_close(st);
        sys_free(st);
    } else if (sys_strcmp(var, "multi") == 0) {
        DIR *dp = opendir(fn);
        ASSERT(dp);
        stream_t *st = stream_chunked_file(dp, fn, DEFAULT_TRACE_EXT);
        var          = sys_getenv("LOTTO_RECORDER_CHUNK_SIZE");
        ASSERT(var);
        rec = trace_chunked_create(st, atoi(var));
        trace_load(rec);
    } else if (sys_strcmp(var, "file") == 0) {
        stream_t *st = stream_file_alloc();
        stream_file_in(st, fn);
        rec = trace_file_create(st);
        ASSERT(rec);
    } else {
        ASSERT(0 && "unknown recorder type");
    }
    return rec;
}

void
cli_trace_save(const trace_t *rec, const char *fn)
{
    char *var = sys_getenv("LOTTO_RECORDER_TYPE");
    if (sys_strcmp(var, "flat") == 0 || sys_strcmp(var, "file") == 0) {
        stream_t *st = stream_file_alloc();
        stream_file_out(st, fn);
        trace_save_to(rec, st);
        stream_close(st);
        sys_free(st);
    } else if (sys_strcmp(var, "multi") == 0) {
        struct stat stats = {0};
        if (stat(fn, &stats) == -1) {
            mkdir(fn, 0700);
        }
        DIR *dp = opendir(fn);
        ASSERT(dp);
        stream_t *st = stream_chunked_file(dp, fn, DEFAULT_TRACE_EXT);
        trace_save_to(rec, st);
        stream_close(st);
    } else {
        ASSERT(0 && "unknown recorder type");
    }
}

uint64_t
cli_trace_last_clk(const char *name)
{
    trace_t *rec = cli_trace_load(name);
    record_t *r  = trace_last(rec);
    if (r == NULL)
        return 0;
    uint64_t result = r->clk;
    trace_destroy(rec);
    return result;
}

void
cli_trace_trim_to_clk(trace_t *trace, clk_t clk)
{
    for (record_t *record = trace_last(trace);
         record != NULL && record->clk > clk;
         trace_forget(trace), record = trace_last(trace)) {}
}

void
cli_trace_trim_to_goal(trace_t *trace, clk_t goal, bool drop_old_config)
{
    ASSERT(goal <= trace_last(trace)->clk);
    for (record_t *record = trace_last(trace);
         record != NULL &&
         (record->clk > goal || (drop_old_config && record->clk == goal &&
                                 record->kind == RECORD_CONFIG));
         trace_forget(trace), record = trace_last(trace)) {}
}

void
cli_trace_trim_to_cat(trace_t *trace, category_t cat)
{
    for (record_t *record = trace_last(trace);
         record != NULL && record->cat != cat;
         trace_forget(trace), record = trace_last(trace)) {}
}

void
cli_trace_trim_to_kind(trace_t *trace, kinds_t kinds)
{
    for (record_t *record = trace_last(trace);
         record != NULL && !(record->kind & kinds);
         trace_forget(trace), record = trace_last(trace)) {}
}

tidset_t *
cli_trace_alternative_tasks(trace_t *trace)
{
    record_t *record = trace_last(trace);
    if (record != NULL) {
        tidset_t *tidset = sys_malloc(sizeof(tidset_t));
        tidset_init(tidset);
        statemgr_unmarshal(record->data, STATE_TYPE_PERSISTENT, false);
        tidset_copy(tidset, get_available_tasks());
        tidset_remove(tidset, record->id);
        return tidset;
    }
    return NULL;
}

void
cli_trace_schedule_task(trace_t *trace, const task_id task)
{
    record_t *record_orig = trace_last(trace);
    ASSERT(record_orig != NULL);
    clk_t clk = record_orig->clk;
    for (record_t *r = record_orig; r != NULL && r->clk == clk &&
                                    (r->kind & (RECORD_SCHED | RECORD_FORCE));
         r = trace_last(trace)) {
        trace_forget(trace);
    }
    record_t *record_new = record_alloc(0);
    record_new->clk      = clk;
    record_new->id       = task;
    record_new->kind     = RECORD_FORCE;
    ENSURE(TRACE_OK == trace_append(trace, record_new) && "could not append");
}

void
cli_trace_init(const char *record, const args_t *args, const char *replay,
               struct flag_val fgoal, bool config, const flags_t *flags)
{
    bool is_replay = replay && replay[0];

    flags_publish(flags);

    char *var = sys_getenv("LOTTO_RECORDER_TYPE");

    if (!var) {
        const char *real_fn = detect_record_type(replay ? replay : record);
        record              = real_fn;
        var                 = sys_getenv("LOTTO_RECORDER_TYPE");
        ASSERT(var);
    }

    bool is_file = sys_strcmp(var, "file") == 0;

    if (is_replay && config) {
        trace_t *rep = cli_trace_load(replay);
        // copy replayer to a temp file
        char tmp_name[PATH_MAX];
        sys_sprintf(tmp_name, "%s.tmp", replay);
        cli_trace_save(rep, tmp_name);
        trace_destroy(rep);
        // trim the replayer
        rep = cli_trace_load(tmp_name);
        clk_t goal =
            fgoal.is_default ? trace_last(rep)->clk : as_uval(fgoal._val);
        cli_trace_trim_to_goal(rep, goal, false);
        record_t *r = record_config(goal);

        if (!is_file && TRACE_OK != trace_append(rep, r))
            ASSERT(0 && "could not append");

        if (!is_file) {
            cli_trace_save(rep, tmp_name);
        }
        trace_destroy(rep);
        // replay the temp file
        sys_setenv("LOTTO_REPLAY", tmp_name, true);
    }
    if (!is_replay) {
        // put initial trace to a temp file
        char tmp_name[PATH_MAX];
        sys_sprintf(tmp_name, "%s.tmp", record);
        trace_t *rec = NULL;
        stream_t *st = NULL;
        if (sys_strcmp(var, "flat") == 0 || is_file) {
            st = stream_file_alloc();
            stream_file_out(st, tmp_name);
            rec = trace_flat_create(st);
        } else if (sys_strcmp(var, "multi") == 0) {
            struct stat stats = {0};
            if (stat(tmp_name, &stats) == -1) {
                mkdir(tmp_name, 0700);
            }
            DIR *dp = opendir(tmp_name);
            ASSERT(dp);
            st = stream_chunked_file(dp, tmp_name, DEFAULT_TRACE_EXT);
            stream_chunked_truncate(st, 0);
            var = sys_getenv("LOTTO_RECORDER_CHUNK_SIZE");
            ASSERT(var);
            rec = trace_chunked_create(st, atoi(var));
        } else {
            ASSERT(0 && "unknown recorder type");
        }
        if (!replay) {
            // add record start to the trace
            record_t *r = record_start(args);
            if (!is_file && TRACE_OK != trace_append(rec, r))
                ASSERT(0 && "could not append");
            r = record_config(0);
            if (!is_file && TRACE_OK != trace_append(rec, r))
                ASSERT(0 && "could not append");
        }
        trace_save(rec);
        trace_destroy(rec);
        stream_close(st);
        sys_free(st);
        // replay the temp file
        sys_setenv("LOTTO_REPLAY", tmp_name, true);
    }
}

void
cli_trace_check_hash(trace_t *t, uint64_t hash)
{
    record_t *r = trace_next(t, RECORD_START);
    statemgr_unmarshal(r->data, STATE_TYPE_START, false);
}

void
cli_trace_copy(const char *from, const char *to)
{
    trace_t *rec = cli_trace_load(from);
    cli_trace_save(rec, to);
    trace_destroy(rec);
}

record_t *
record_start(const args_t *args)
{
    record_t *r = record_alloc(statemgr_size(STATE_TYPE_START));
    r->kind     = RECORD_START;
    statemgr_marshal(r->data, STATE_TYPE_START);

    return r;
}

void
env_set_trace_type(const char *type)
{
    envvar_t vars[] = {{"LOTTO_RECORDER_TYPE", .sval = type}, {NULL}};
    envvar_set(vars, true);
}

void
env_set_recorder_chunk_size(uint64_t size)
{
    envvar_t vars[] = {{"LOTTO_RECORDER_CHUNK_SIZE", .uval = size}, {NULL}};
    envvar_set(vars, true);
}

void
set_recorder_chunk_size(const char *dn)
{
    int chunk_size    = 0;
    stream_t *st      = NULL;
    DIR *dp           = NULL;
    trace_t *recorder = NULL;
    record_t *record  = NULL;

    dp = opendir(dn);
    ASSERT(dp);

    st = stream_chunked_file(dp, dn, DEFAULT_TRACE_EXT);
    ASSERT(st);

    recorder = trace_chunked_create(st, UINT64_MAX);

    while ((record = trace_next(recorder, RECORD_ANY)) != NULL) {
        chunk_size++;
        trace_advance(recorder);
    }
    trace_destroy(recorder);

    env_set_trace_type("multi");
    env_set_recorder_chunk_size(chunk_size);

    closedir(dp);

    return;
}

const char *
detect_record_type(const char *fn)
{
    const char *real_fn = NULL;
    // try auto-detect recorder type and chunk size if not specified
    DIR *tmp_dp = opendir(fn);
    if (tmp_dp) {
        closedir(tmp_dp);
        real_fn = fn;
        env_set_trace_type("multi");
        set_recorder_chunk_size(real_fn);
        return real_fn;
    }

    char *multi_fn = strstr(fn, "0" DEFAULT_TRACE_EXT);
    if (multi_fn && sys_strlen(fn) == sys_strlen("0" DEFAULT_TRACE_EXT)) {
        // multi: filename 0.trace
        real_fn = dirname((char *)fn);
        ASSERT(real_fn);
        env_set_trace_type("multi");
        set_recorder_chunk_size(real_fn);
    } else {
        // flat
        real_fn = fn;
        env_set_trace_type("flat");
    }

    return real_fn;
}

void
record_print(const record_t *r, int i)
{
    char clk_str[256] = {0};
    sys_sprintf(clk_str, "%lu", r->clk);
    log_println("====================");
    log_println("RECORD %d", i);
    log_println("  clock:    %s", clk_str);
    log_println("  task:     %lu", r->id);
    log_println("  category: %s", category_str(r->cat));
    log_println("  reason:   %s", reason_str(r->reason));
    log_println("  kind:     %s", kind_str(r->kind));
    log_println("  pc:       %" PRIxPTR, r->pc);
    log_println("  ------------------\n");
    switch (r->kind) {
        case RECORD_SCHED:
            if (r->size > 0) {
                statemgr_unmarshal(r->data, STATE_TYPE_PERSISTENT, true);
                statemgr_print(STATE_TYPE_PERSISTENT);
            }
            break;
        case RECORD_SHUTDOWN_LOCK:
        case RECORD_EXIT:
            if (r->size > 0) {
                statemgr_unmarshal(r->data, STATE_TYPE_FINAL, true);
                statemgr_print(STATE_TYPE_FINAL);
            }
            break;
        case RECORD_START:
            statemgr_unmarshal(r->data, STATE_TYPE_START, true);
            statemgr_print(STATE_TYPE_START);
            break;
        case RECORD_CONFIG: {
            statemgr_unmarshal(r->data, STATE_TYPE_CONFIG, true);
            statemgr_print(STATE_TYPE_CONFIG);
            break;
        }
        case RECORD_FORCE:
        case RECORD_INFO:
            break;
    }
}

args_t *
record_args(const record_t *r)
{
    ASSERT(r->kind == RECORD_START);
    statemgr_record_unmarshal(r);
    return &get_exec_info()->args;
}
