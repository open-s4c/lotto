#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/record.h>
#include <lotto/base/trace.h>
#include <lotto/driver/exec.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/prng.h>
#include <lotto/driver/record.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/trace.h>
#include <lotto/driver/utils.h>
#include <lotto/engine/state.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/qemu_snapshot/config.h>
#include <lotto/modules/qemu_snapshot/events.h>
#include <lotto/modules/qemu_snapshot/final.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <lotto/sys/unistd.h>
#include <sys/stat.h>

#define SNAPSHOT_TRACE_BASENAME        "snapshot.trace"
#define SNAPSHOT_DRIVE_BASENAME        "snapshot.qcow2"
#define SNAPSHOT_RESUME_TRACE_BASENAME "from-snap.trace"
#define SNAPSHOT_RESUME_DRIVE_BASENAME "from-snap.qcow2"

typedef struct snap_source {
    record_t *start;
    record_t *config;
    record_t *snapshot;
    char *snapshot_name;
    char *snapshot_drive;
} snap_source_t;

static bool
_ensure_directory(const char *dir)
{
    struct stat st = {0};

    if (stat(dir, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    return mkdir(dir, 0755) == 0;
}

static args_t
_args_dup(const args_t *src)
{
    args_t dst = {
        .argc = src->argc,
        .argv = sys_calloc((size_t)src->argc + 1, sizeof(char *)),
        .arg0 = src->arg0 ? sys_strdup(src->arg0) : NULL,
    };

    for (int i = 0; i < src->argc; i++) {
        dst.argv[i] = sys_strdup(src->argv[i]);
    }
    dst.argv[src->argc] = NULL;
    if (dst.arg0 == NULL && dst.argc > 0) {
        dst.arg0 = sys_strdup(dst.argv[0]);
    }
    return dst;
}

static void
_args_free(args_t *args)
{
    if (args == NULL) {
        return;
    }

    if (args->argv != NULL) {
        for (int i = 0; i < args->argc; i++) {
            sys_free(args->argv[i]);
        }
        sys_free(args->argv);
    }
    sys_free(args->arg0);
    *args = (args_t){0};
}

static char *
_drive_file_path(const char *spec)
{
    const char *file = strstr(spec, "file=");
    if (file == NULL) {
        return NULL;
    }

    file += sys_strlen("file=");
    const char *end = strchr(file, ',');
    if (end == NULL) {
        end = file + sys_strlen(file);
    }
    return sys_strndup(file, (size_t)(end - file));
}

static void
_snapshot_trace_path(char *path, const char *temporary_directory)
{
    sys_sprintf(path, "%s/%s", temporary_directory, SNAPSHOT_TRACE_BASENAME);
}

static void
_snapshot_drive_path(char *path, const char *temporary_directory)
{
    sys_sprintf(path, "%s/%s", temporary_directory, SNAPSHOT_DRIVE_BASENAME);
}

static char *
_replace_drive_file(const char *spec, const char *new_file)
{
    const char *file = strstr(spec, "file=");
    if (file == NULL) {
        return sys_strdup(spec);
    }

    const char *path_start = file + sys_strlen("file=");
    const char *path_end   = strchr(path_start, ',');
    if (path_end == NULL) {
        path_end = path_start + sys_strlen(path_start);
    }

    size_t prefix_len = (size_t)(path_start - spec);
    size_t suffix_len = sys_strlen(path_end);
    size_t total_len  = prefix_len + sys_strlen(new_file) + suffix_len + 1;
    char *result      = sys_malloc(total_len);

    sys_memcpy(result, spec, prefix_len);
    sys_strcpy(result + prefix_len, new_file);
    sys_strcpy(result + prefix_len + sys_strlen(new_file), path_end);
    return result;
}

static char *
_find_snapshot_drive(const args_t *args)
{
    for (int i = 0; i + 1 < args->argc; i++) {
        if (sys_strcmp(args->argv[i], "-drive") != 0) {
            continue;
        }

        const char *spec = args->argv[i + 1];
        if (strstr(spec, "format=qcow2") == NULL) {
            continue;
        }

        char *path = _drive_file_path(spec);
        if (path != NULL) {
            return path;
        }
    }

    return NULL;
}

static int
_args_replace_snapshot_drive(args_t *args, const char *snapshot_drive_path)
{
    int drive_idx = -1;

    for (int i = 0; i < args->argc; i++) {
        if (i + 1 < args->argc && sys_strcmp(args->argv[i], "-drive") == 0 &&
            drive_idx < 0 &&
            strstr(args->argv[i + 1], "format=qcow2") != NULL) {
            drive_idx = i + 1;
        }
    }

    if (drive_idx < 0) {
        return 1;
    }

    char *updated_drive =
        _replace_drive_file(args->argv[drive_idx], snapshot_drive_path);
    sys_free(args->argv[drive_idx]);
    args->argv[drive_idx] = updated_drive;
    return 0;
}

static int
_args_enable_snapshot(args_t *args, const char *snapshot_name,
                      const char *snapshot_drive_copy)
{
    int loadvm_idx = -1;

    if (_args_replace_snapshot_drive(args, snapshot_drive_copy) != 0) {
        sys_fprintf(stderr,
                    "error: recorded QEMU command line has no qcow2 drive to "
                    "resume from snapshot '%s'\n",
                    snapshot_name);
        return 1;
    }

    for (int i = 0; i < args->argc; i++) {
        if (sys_strcmp(args->argv[i], "-loadvm") == 0 && i + 1 < args->argc) {
            loadvm_idx = i + 1;
            break;
        }
    }

    if (loadvm_idx >= 0) {
        sys_free(args->argv[loadvm_idx]);
        args->argv[loadvm_idx] = sys_strdup(snapshot_name);
        return 0;
    }

    char **argv = sys_calloc((size_t)args->argc + 3, sizeof(char *));
    for (int i = 0; i < args->argc; i++) {
        argv[i] = args->argv[i];
    }
    argv[args->argc++] = sys_strdup("-loadvm");
    argv[args->argc++] = sys_strdup(snapshot_name);
    argv[args->argc]   = NULL;
    sys_free(args->argv);
    args->argv = argv;
    return 0;
}

static int
_load_source(const char *input, snap_source_t *source, args_t *recorded_args)
{
    if (input == NULL || input[0] == '\0') {
        input = "replay.trace";
    }
    if (access(input, R_OK) != 0) {
        sys_fprintf(stderr, "error: could not read input trace '%s': %s\n",
                    input, strerror(errno));
        return 1;
    }

    trace_t *trace  = cli_trace_load(input);
    record_t *final = trace_last(trace);
    if (final == NULL || final->kind != RECORD_EXIT || final->size == 0) {
        sys_fprintf(stderr, "error: trace '%s' has no final state\n", input);
        trace_destroy(trace);
        return 1;
    }

    statemgr_unmarshal(final->data, STATE_TYPE_FINAL, true);
    const struct qemu_snapshot_final_state *snapshot_final =
        qemu_snapshot_final_state();
    if (!snapshot_final->valid || !snapshot_final->success ||
        snapshot_final->name[0] == '\0') {
        sys_fprintf(
            stderr,
            "error: trace '%s' does not contain a successful snapshot\n",
            input);
        trace_destroy(trace);
        return 1;
    }

    record_t *start    = NULL;
    record_t *config   = NULL;
    record_t *snapshot = NULL;

    while (true) {
        record_t *cur = trace_next(trace, RECORD_ANY);
        if (cur == NULL) {
            break;
        }

        if (start == NULL && cur->kind == RECORD_START) {
            start               = record_clone(cur);
            args_t *stored_args = record_args(cur);
            *recorded_args      = _args_dup(stored_args);
        } else if (config == NULL && cur->kind == RECORD_CONFIG) {
            config = record_clone(cur);
        } else if (cur->kind == RECORD_SCHED &&
                   cur->type_id == EVENT_QEMU_SNAPSHOT_DONE &&
                   cur->clk == snapshot_final->clk) {
            snapshot = record_clone(cur);
        }

        trace_advance(trace);
    }

    trace_destroy(trace);

    if (start == NULL || config == NULL || snapshot == NULL) {
        sys_fprintf(stderr,
                    "error: trace '%s' is missing start/config/snapshot "
                    "records\n",
                    input);
        return 1;
    }

    source->start          = start;
    source->config         = config;
    source->snapshot       = snapshot;
    source->snapshot_name  = sys_strdup(snapshot_final->name);
    source->snapshot_drive = _find_snapshot_drive(recorded_args);
    if (source->snapshot_drive == NULL) {
        sys_fprintf(stderr,
                    "error: trace '%s' does not reference a qcow2 snapshot "
                    "drive\n",
                    input);
        return 1;
    }

    return 0;
}

static bool
_trace_has_successful_snapshot(const char *input)
{
    if (input == NULL || input[0] == '\0' || access(input, R_OK) != 0) {
        return false;
    }

    trace_t *trace  = cli_trace_load(input);
    record_t *final = trace != NULL ? trace_last(trace) : NULL;
    bool ok         = false;

    if (final != NULL && final->kind == RECORD_EXIT && final->size > 0) {
        statemgr_unmarshal(final->data, STATE_TYPE_FINAL, true);
        const struct qemu_snapshot_final_state *snapshot_final =
            qemu_snapshot_final_state();
        ok = snapshot_final->valid && snapshot_final->success &&
             snapshot_final->name[0] != '\0';
        if (ok) {
            ok = false;
            while (true) {
                record_t *cur = trace_next(trace, RECORD_ANY);
                if (cur == NULL) {
                    break;
                }
                if (cur->kind == RECORD_SCHED &&
                    cur->type_id == EVENT_QEMU_SNAPSHOT_DONE &&
                    cur->clk == snapshot_final->clk) {
                    ok = true;
                    break;
                }
                trace_advance(trace);
            }
        }
    }

    if (trace != NULL) {
        trace_destroy(trace);
    }
    return ok;
}

static void
_free_source(snap_source_t *source)
{
    if (source == NULL) {
        return;
    }

    sys_free(source->start);
    sys_free(source->config);
    sys_free(source->snapshot);
    sys_free(source->snapshot_name);
    sys_free(source->snapshot_drive);
    *source = (snap_source_t){0};
}

static int
_write_resume_trace(const char *source_trace, const char *path,
                    const snap_source_t *source, const char *snapshot_copy_path,
                    uint64_t seed)
{
    cli_trace_copy(source_trace, path);
    trace_t *trace = cli_trace_load(path);
    if (trace == NULL) {
        sys_fprintf(stderr, "error: could not load temporary trace '%s'\n",
                    path);
        return 1;
    }
    trace_clear(trace);

    args_t start_args = {0};
    start_args        = _args_dup(record_args(source->start));
    if (_args_enable_snapshot(&start_args, source->snapshot_name,
                              snapshot_copy_path) != 0) {
        _args_free(&start_args);
        trace_destroy(trace);
        return 1;
    }

    if (TRACE_OK != trace_append(trace, record_start(&start_args)) ||
        TRACE_OK != trace_append(trace, record_clone(source->config)) ||
        TRACE_OK != trace_append(trace, record_clone(source->snapshot))) {
        sys_fprintf(stderr,
                    "error: could not append snapshot resume records to '%s'\n",
                    path);
        _args_free(&start_args);
        trace_destroy(trace);
        return 1;
    }
    _args_free(&start_args);

    statemgr_record_unmarshal(source->config);
    struct qemu_snapshot_config_state *snapshot_cfg =
        qemu_snapshot_config_state();
    prng()->seed                   = (uint32_t)seed;
    snapshot_cfg->enabled          = true;
    snapshot_cfg->clk              = source->snapshot->clk + 1;
    snapshot_cfg->snapshot_valid   = true;
    snapshot_cfg->snapshot_success = true;
    snapshot_cfg->snapshot_clk     = source->snapshot->clk;
    sys_strcpy(snapshot_cfg->snapshot_name, source->snapshot_name);
    record_t *conf = record_config(source->snapshot->clk);
    if (TRACE_OK != trace_append(trace, conf)) {
        sys_fprintf(stderr, "error: could not append updated config to '%s'\n",
                    path);
        trace_destroy(trace);
        return 1;
    }

    cli_trace_save(trace, path);
    trace_destroy(trace);
    return 0;
}

static int
_rewrite_output_trace_with_snapshot_prefix(const char *output_trace,
                                           const char *resume_trace,
                                           const record_t *snapshot_record,
                                           const char *snapshot_name)
{
    trace_t *prefix = cli_trace_load(resume_trace);
    trace_t *source = cli_trace_load(output_trace);
    trace_t *trace  = cli_trace_load(output_trace);
    if (prefix == NULL || source == NULL || trace == NULL) {
        sys_fprintf(stderr, "error: could not load output trace '%s'\n",
                    output_trace);
        if (prefix != NULL) {
            trace_destroy(prefix);
        }
        if (source != NULL) {
            trace_destroy(source);
        }
        if (trace != NULL) {
            trace_destroy(trace);
        }
        return 1;
    }

    record_t *start         = NULL;
    record_t *config_before = NULL;
    record_t *snapshot_done = NULL;
    record_t *config_after  = NULL;
    trace_clear(trace);

    while (true) {
        record_t *cur = trace_next(prefix, RECORD_ANY);
        if (cur == NULL) {
            break;
        }

        if (start == NULL && cur->kind == RECORD_START) {
            start = record_clone(cur);
        } else if (config_before == NULL && cur->kind == RECORD_CONFIG) {
            config_before = record_clone(cur);
        } else if (snapshot_done == NULL && cur->kind == RECORD_SCHED &&
                   cur->type_id == EVENT_QEMU_SNAPSHOT_DONE) {
            snapshot_done = record_clone(cur);
        } else if (snapshot_done != NULL && config_after == NULL &&
                   cur->kind == RECORD_CONFIG) {
            config_after = record_clone(cur);
            break;
        }

        trace_advance(prefix);
    }

    if (start == NULL || config_before == NULL || snapshot_done == NULL ||
        config_after == NULL) {
        sys_fprintf(stderr,
                    "error: resume trace '%s' is missing snapshot "
                    "prefix records\n",
                    resume_trace);
        trace_destroy(trace);
        trace_destroy(prefix);
        trace_destroy(source);
        return 1;
    }

    config_before->clk = 0;
    snapshot_done->clk = snapshot_record->clk;
    config_after->clk  = snapshot_record->clk;

    if (TRACE_OK != trace_append(trace, start) ||
        TRACE_OK != trace_append(trace, config_before) ||
        TRACE_OK != trace_append(trace, snapshot_done) ||
        TRACE_OK != trace_append(trace, config_after)) {
        sys_fprintf(stderr, "error: could not rewrite output trace '%s'\n",
                    output_trace);
        trace_destroy(trace);
        trace_destroy(prefix);
        trace_destroy(source);
        return 1;
    }

    bool after_config = false;
    record_t *cur     = trace_next(source, RECORD_ANY);
    while (cur != NULL) {
        if (!after_config) {
            if (cur->kind == RECORD_CONFIG) {
                after_config = true;
            }
            trace_advance(source);
            cur = trace_next(source, RECORD_ANY);
            continue;
        }

        if (TRACE_OK != trace_append(trace, record_clone(cur))) {
            sys_fprintf(stderr, "error: could not rewrite output trace '%s'\n",
                        output_trace);
            trace_destroy(trace);
            trace_destroy(prefix);
            trace_destroy(source);
            return 1;
        }
        trace_advance(source);
        cur = trace_next(source, RECORD_ANY);
    }

    record_t *out_final = trace_last(trace);
    if (out_final != NULL && out_final->kind == RECORD_EXIT &&
        out_final->size > 0) {
        statemgr_unmarshal(out_final->data, STATE_TYPE_FINAL, true);
        qemu_snapshot_final_note(snapshot_name, true);
        qemu_snapshot_final_set_clk(snapshot_record->clk);
        (void)statemgr_marshal(out_final->data, STATE_TYPE_FINAL);
    }

    cli_trace_save(trace, output_trace);
    trace_destroy(trace);
    trace_destroy(prefix);
    trace_destroy(source);
    return 0;
}

static flags_t *
_default_flags()
{
    flags_t *flags = run_default_flags();
    flags_set_default(flags, flag_input(), sval(SNAPSHOT_TRACE_BASENAME));
    return flags;
}

static int
_rewrite_snapshot_trace(const char *source_trace, const char *snapshot_trace,
                        const char *snapshot_drive)
{
    trace_t *source = cli_trace_load(source_trace);
    if (source == NULL) {
        sys_fprintf(stderr, "error: could not load trace '%s'\n", source_trace);
        return 1;
    }

    cli_trace_copy(source_trace, snapshot_trace);
    trace_t *backup = cli_trace_load(snapshot_trace);
    if (backup == NULL) {
        sys_fprintf(stderr, "error: could not prepare trace '%s'\n",
                    snapshot_trace);
        trace_destroy(source);
        return 1;
    }

    trace_clear(backup);

    while (true) {
        record_t *cur = trace_next(source, RECORD_ANY);
        if (cur == NULL) {
            break;
        }

        if (cur->kind == RECORD_START) {
            args_t *stored_args = record_args(cur);
            args_t backup_args  = _args_dup(stored_args);
            if (_args_replace_snapshot_drive(&backup_args, snapshot_drive) !=
                0) {
                sys_fprintf(stderr,
                            "error: trace '%s' has no qcow2 drive to back up\n",
                            source_trace);
                _args_free(&backup_args);
                trace_destroy(backup);
                trace_destroy(source);
                return 1;
            }
            if (TRACE_OK != trace_append(backup, record_start(&backup_args))) {
                sys_fprintf(stderr, "error: could not rewrite trace '%s'\n",
                            snapshot_trace);
                _args_free(&backup_args);
                trace_destroy(backup);
                trace_destroy(source);
                return 1;
            }
            _args_free(&backup_args);
        } else if (TRACE_OK != trace_append(backup, record_clone(cur))) {
            sys_fprintf(stderr, "error: could not rewrite trace '%s'\n",
                        snapshot_trace);
            trace_destroy(backup);
            trace_destroy(source);
            return 1;
        }

        trace_advance(source);
    }

    cli_trace_save(backup, snapshot_trace);
    trace_destroy(backup);
    trace_destroy(source);
    return 0;
}

static int
_backup_snapshot_artifacts(const char *trace_path,
                           const char *temporary_directory)
{
    snap_source_t source = {0};
    args_t recorded_args = {0};
    int err              = _load_source(trace_path, &source, &recorded_args);
    if (err != 0) {
        _free_source(&source);
        _args_free(&recorded_args);
        return err;
    }

    char snapshot_trace[PATH_MAX];
    char snapshot_drive[PATH_MAX];
    _snapshot_trace_path(snapshot_trace, temporary_directory);
    _snapshot_drive_path(snapshot_drive, temporary_directory);

    if (cp(source.snapshot_drive, snapshot_drive) != 0) {
        sys_fprintf(
            stderr,
            "error: could not back up snapshot drive '%s' to '%s': %s\n",
            source.snapshot_drive, snapshot_drive, strerror(errno));
        _free_source(&source);
        _args_free(&recorded_args);
        return 1;
    }

    err = _rewrite_snapshot_trace(trace_path, snapshot_trace, snapshot_drive);
    _free_source(&source);
    _args_free(&recorded_args);
    return err;
}

static int
_snapshot_post_run(const args_t *args, const flags_t *flags, int ret)
{
    (void)args;

    const char *trace_path = flags_get_sval(flags, flag_output());
    if (trace_path == NULL || trace_path[0] == '\0') {
        return ret;
    }
    if (!_trace_has_successful_snapshot(trace_path)) {
        return ret;
    }

    const char *temporary_directory =
        flags_get_sval(flags, flag_temporary_directory());
    if (temporary_directory == NULL || temporary_directory[0] == '\0') {
        return ret;
    }

    int backup_err =
        _backup_snapshot_artifacts(trace_path, temporary_directory);
    if (backup_err != 0) {
        return ret != 0 ? ret : backup_err;
    }

    return ret;
}

static int
_prepare_round(const snap_source_t *source, const char *temporary_directory,
               const char *source_trace, const char *trace_path,
               const char *snapshot_copy_path, uint64_t seed)
{
    if (!_ensure_directory(temporary_directory)) {
        sys_fprintf(stderr,
                    "error: could not create temporary directory '%s'\n",
                    temporary_directory);
        return 1;
    }

    if (cp(source->snapshot_drive, snapshot_copy_path) != 0) {
        sys_fprintf(
            stderr, "error: could not copy snapshot drive '%s' to '%s': %s\n",
            source->snapshot_drive, snapshot_copy_path, strerror(errno));
        return 1;
    }

    return _write_resume_trace(source_trace, trace_path, source,
                               snapshot_copy_path, seed);
}

int
snap_stress(args_t *args, flags_t *flags)
{
    (void)args;

    const char *temporary_directory =
        flags_get_sval(flags, flag_temporary_directory());
    char snapshot_trace_path[PATH_MAX];
    char trace_path[PATH_MAX];
    char snapshot_copy_path[PATH_MAX];
    _snapshot_trace_path(snapshot_trace_path, temporary_directory);
    sys_sprintf(trace_path, "%s/%s", temporary_directory,
                SNAPSHOT_RESUME_TRACE_BASENAME);
    sys_sprintf(snapshot_copy_path, "%s/%s", temporary_directory,
                SNAPSHOT_RESUME_DRIVE_BASENAME);

    const char *input = flags_get_sval(flags, flag_input());
    if (input == NULL || input[0] == '\0' ||
        flags_get(flags, flag_input()).is_default) {
        input = snapshot_trace_path;
    }

    snap_source_t source = {0};
    args_t replay_args   = {0};
    int err              = _load_source(input, &source, &replay_args);
    if (err != 0) {
        _free_source(&source);
        _args_free(&replay_args);
        return err;
    }

    execute_resolve_replay_args(&replay_args, flags);

    flags_set_by_opt(flags, flag_input(), sval(trace_path));

    struct flag_val seed = flags_get(flags, flag_seed());
    uint64_t rounds      = flags_get_uval(flags, flag_rounds());

    for (uint64_t i = 0; i < rounds; i++) {
        round_print(flags, i);

        uint64_t round_seed =
            seed.is_default ? (uint64_t)now() : as_uval(seed._val);
        flags_set_by_opt(flags, flag_seed(), uval(round_seed));

        err = _prepare_round(&source, temporary_directory, input, trace_path,
                             snapshot_copy_path, round_seed);
        if (err != 0) {
            break;
        }

        args_t run_args = _args_dup(&replay_args);
        err             = _args_enable_snapshot(&run_args, source.snapshot_name,
                                                snapshot_copy_path);
        if (err != 0) {
            _args_free(&run_args);
            break;
        }

        err = run_once(&run_args, flags);
        _args_free(&run_args);

        int rewrite_err = _rewrite_output_trace_with_snapshot_prefix(
            flags_get_sval(flags, flag_output()), trace_path, source.snapshot,
            source.snapshot_name);
        if (rewrite_err != 0) {
            err = rewrite_err;
            break;
        }
        if (err != 0) {
            break;
        }

        if (rounds > 1) {
            adjust(flags_get_sval(flags, flag_output()));
        }

        if (seed.is_default) {
            seed._val = uval((uint64_t)now());
        } else {
            seed._val = uval(as_uval(seed._val) + 1);
        }
        flags_set_by_opt(flags, flag_seed(), seed._val);
    }

    _free_source(&source);
    return err;
}

ON_DRIVER_REGISTER_COMMANDS({
    run_set_post_run_hook(_snapshot_post_run);
    flag_t sel[] = {flag_input(),
                    flag_output(),
                    flag_verbose(),
                    flag_rounds(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_before_run(),
                    flag_after_run(),
                    flag_logger_file(),
                    0};
    subcmd_register(
        snap_stress, "snap-stress", "",
        "Resume repeated QEMU stress runs from a saved internal snapshot", true,
        sel, _default_flags, SUBCMD_GROUP_TRACE);
})
