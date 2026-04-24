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
#include <lotto/modules/qemu_snapshot/events.h>
#include <lotto/modules/qemu_snapshot/final.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <sys/stat.h>

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

static void
_args_enable_snapshot(args_t *args, const char *snapshot_name,
                      const char *snapshot_drive_copy)
{
    int drive_idx  = -1;
    int loadvm_idx = -1;

    for (int i = 0; i < args->argc; i++) {
        if (i + 1 < args->argc && sys_strcmp(args->argv[i], "-drive") == 0 &&
            drive_idx < 0 &&
            strstr(args->argv[i + 1], "format=qcow2") != NULL) {
            drive_idx = i + 1;
        }

        if (sys_strcmp(args->argv[i], "-loadvm") == 0 && i + 1 < args->argc) {
            loadvm_idx = i + 1;
        }
    }

    ASSERT(drive_idx >= 0 && "snapshot resume requires a qcow2 drive");

    char *updated_drive =
        _replace_drive_file(args->argv[drive_idx], snapshot_drive_copy);
    sys_free(args->argv[drive_idx]);
    args->argv[drive_idx] = updated_drive;

    if (loadvm_idx >= 0) {
        sys_free(args->argv[loadvm_idx]);
        args->argv[loadvm_idx] = sys_strdup(snapshot_name);
        return;
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
}

static int
_load_source(const char *input, snap_source_t *source, args_t *recorded_args)
{
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
                    const snap_source_t *source, uint64_t seed)
{
    cli_trace_copy(source_trace, path);
    trace_t *trace = cli_trace_load(path);
    trace_clear(trace);

    ENSURE(TRACE_OK == trace_append(trace, record_clone(source->start)));
    ENSURE(TRACE_OK == trace_append(trace, record_clone(source->config)));
    ENSURE(TRACE_OK == trace_append(trace, record_clone(source->snapshot)));

    statemgr_record_unmarshal(source->config);
    prng()->seed   = (uint32_t)seed;
    record_t *conf = record_config(source->snapshot->clk);
    ENSURE(TRACE_OK == trace_append(trace, conf));

    cli_trace_save(trace, path);
    trace_destroy(trace);
    return 0;
}

static flags_t *
_default_flags()
{
    return run_default_flags();
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

    return _write_resume_trace(source_trace, trace_path, source, seed);
}

int
snap_stress(args_t *args, flags_t *flags)
{
    (void)args;

    const char *input    = flags_get_sval(flags, flag_input());
    snap_source_t source = {0};
    args_t replay_args   = {0};
    int err              = _load_source(input, &source, &replay_args);
    if (err != 0) {
        _free_source(&source);
        _args_free(&replay_args);
        return err;
    }

    execute_resolve_replay_args(&replay_args, flags);

    const char *temporary_directory =
        flags_get_sval(flags, flag_temporary_directory());
    char trace_path[PATH_MAX];
    char snapshot_copy_path[PATH_MAX];
    sys_sprintf(trace_path, "%s/from-snap.trace", temporary_directory);
    sys_sprintf(snapshot_copy_path, "%s/%s", temporary_directory,
                strrchr(source.snapshot_drive, '/') ?
                    strrchr(source.snapshot_drive, '/') + 1 :
                    source.snapshot_drive);

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
        _args_enable_snapshot(&run_args, source.snapshot_name,
                              snapshot_copy_path);

        err = run_once(&run_args, flags);
        _args_free(&run_args);
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
    flag_t sel[] = {flag_input(),
                    flag_output(),
                    flag_seed(),
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
        sel, _default_flags, SUBCMD_GROUP_RUN);
})
