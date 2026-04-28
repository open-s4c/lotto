#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "state.h"
#include <lotto/driver/exec.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/subcmd.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

#define DICE_PLUGIN_MODULES "DICE_PLUGIN_MODULES"
#define MAX_LIST_STR        ((size_t)(32 * 1024))
#define LD_PRELOAD_ENV      "LD_PRELOAD"
#define LOTTO_CLI_PRELOAD   "LOTTO_CLI_PRELOAD"

DECLARE_COMMAND_FLAG(QEMU_BIN, "", LOTTO_MODULE_FLAG("bin"), "PATH",
                     "path to qemu-system-aarch64", flag_sval(""))
FLAG_GETTER(qemu_bin, QEMU_BIN)
DECLARE_COMMAND_FLAG(QEMU_PLUGIN_DEBUG, "", LOTTO_MODULE_FLAG("plugin-debug"),
                     "", "append '-d plugin' for qemu plugin debugging",
                     flag_off())
FLAG_GETTER(qemu_plugin_debug, QEMU_PLUGIN_DEBUG)
DECLARE_COMMAND_FLAG(QEMU_MEM, "", LOTTO_MODULE_FLAG("mem"), "SIZE",
                     "default qemu memory size for injected args",
                     flag_sval(""))
FLAG_GETTER(qemu_mem, QEMU_MEM)
DECLARE_COMMAND_FLAG(QEMU_CPU, "", LOTTO_MODULE_FLAG("cpu"), "N",
                     "default qemu cpu count for injected args", flag_sval(""))
FLAG_GETTER(qemu_cpu, QEMU_CPU)
DECLARE_COMMAND_FLAG(QEMU_NO_COMMON_ARGS, "",
                     LOTTO_MODULE_FLAG("no-common-arguments"), "",
                     "do not inject common QEMU args", flag_off())
FLAG_GETTER(qemu_no_common_args, QEMU_NO_COMMON_ARGS)
DECLARE_COMMAND_FLAG(QEMU_PLUGINS, "", LOTTO_MODULE_FLAG("plugins"), "on|off",
                     "whether to pass qemu -plugin arguments", flag_sval("on"))
FLAG_GETTER(qemu_plugins, QEMU_PLUGINS)

static const char *
_existing_path(const char *preferred, const char *fallback)
{
    if (preferred && access(preferred, X_OK) == 0) {
        return preferred;
    }
    if (fallback && access(fallback, X_OK) == 0) {
        return fallback;
    }
    return preferred;
}

static const char *
_existing_file(const char *preferred, const char *fallback)
{
    if (preferred && access(preferred, R_OK) == 0) {
        return preferred;
    }
    if (fallback && access(fallback, R_OK) == 0) {
        return fallback;
    }
    return preferred;
}

static void
_resolve_lotto_anchor(const args_t *args, char *resolved_path)
{
    ASSERT(resolved_path != NULL);

    if (args != NULL && args->arg0 != NULL && args->arg0[0] != '\0') {
        const char *base = strrchr(args->arg0, '/');
        base             = base != NULL ? base + 1 : args->arg0;
        if (strstr(base, "lotto") != NULL &&
            realpath(args->arg0, resolved_path) != NULL) {
            return;
        }
    }

    if (realpath("/proc/self/exe", resolved_path) != NULL) {
        return;
    }

    if (args != NULL && args->arg0 != NULL && args->arg0[0] != '\0') {
        sys_sprintf(resolved_path, "%s", args->arg0);
        return;
    }

    resolved_path[0] = '\0';
}

static const char *
_plugin_path(const char *plugin_dir, const char *fallback_dir,
             const char *filename)
{
    static char selected[4][PATH_MAX];
    static unsigned int idx;

    char preferred[PATH_MAX];
    char fallback[PATH_MAX];

    sys_sprintf(preferred, "%s/%s", plugin_dir, filename);
    sys_sprintf(fallback, "%s/%s", fallback_dir, filename);

    idx              = (idx + 1U) % 4U;
    const char *path = _existing_file(preferred, fallback);
    sys_sprintf(selected[idx], "%s", path);
    return selected[idx];
}

static void
_append_list_entry(char *buf, size_t buf_size, size_t *len, const char *entry)
{
    int written;
    char buf_copy[MAX_LIST_STR];

    if (entry == NULL || entry[0] == '\0') {
        return;
    }
    if (*len > 0) {
        sys_snprintf(buf_copy, sizeof(buf_copy), "%s", buf);
        char *saveptr;
        for (const char *it = strtok_r(buf_copy, ":", &saveptr); it != NULL;
             it             = strtok_r(NULL, ":", &saveptr)) {
            if (sys_strcmp(it, entry) == 0) {
                return;
            }
        }
    }
    if (*len > 0) {
        written = sys_snprintf(buf + *len, buf_size - *len, ":%s", entry);
    } else {
        written = sys_snprintf(buf + *len, buf_size - *len, "%s", entry);
    }
    ASSERT(written >= 0 && (size_t)written < buf_size - *len);
    *len += (size_t)written;
}

static void
_append_runtime_variant_from_driver(char *buf, size_t buf_size, size_t *len,
                                    const char *driver_entry,
                                    const char *runtime_prefix)
{
    static const char *driver_prefix = "lotto-driver-";
    char converted[MAX_LIST_STR];
    const char *needle = strstr(driver_entry, driver_prefix);
    if (needle == NULL) {
        return;
    }

    size_t pre_len = (size_t)(needle - driver_entry);
    int written    = sys_snprintf(converted, sizeof(converted), "%.*s%s%s",
                                  (int)pre_len, driver_entry, runtime_prefix,
                                  needle + sys_strlen(driver_prefix));
    ASSERT(written >= 0 && (size_t)written < sizeof(converted));
    if (access(converted, R_OK) == 0) {
        _append_list_entry(buf, buf_size, len, converted);
    }
}

static void
_build_runtime_plugin_modules(char *buf, size_t buf_size,
                              const char *runtime_qemu_plugin_path,
                              const char *module_dir,
                              const char *module_fallback_dir,
                              bool verbose_enabled)
{
    size_t len = 0;

    buf[0] = '\0';
    _append_list_entry(buf, buf_size, &len, runtime_qemu_plugin_path);

    const char *variants[] = {
        verbose_enabled ? "lotto-runtime-dbg-qemu_gdb.so" :
                          "lotto-runtime-qemu_gdb.so",
        verbose_enabled ? "lotto-runtime-dbg-qemu_profile.so" :
                          "lotto-runtime-qemu_profile.so",
        verbose_enabled ? "lotto-runtime-dbg-qemu_snapshot.so" :
                          "lotto-runtime-qemu_snapshot.so",
    };

    for (size_t i = 0; i < sizeof(variants) / sizeof(variants[0]); i++) {
        const char *path =
            _plugin_path(module_dir, module_fallback_dir, variants[i]);
        if (path != NULL && access(path, R_OK) == 0) {
            _append_list_entry(buf, buf_size, &len, path);
        }
    }
}

static bool
_env_entry_has_key(const char *entry, const char *key)
{
    size_t key_len = sys_strlen(key);
    return sys_strncmp(entry, key, key_len) == 0 && entry[key_len] == '=';
}

static int
_execvpe_with_filtered_env(char *const argv[], const char *runtime_modules)
{
    extern char **environ;
    size_t envc = 0;
    while (environ[envc] != NULL) {
        envc++;
    }

    char **envp = sys_calloc(envc + 2, sizeof(char *));
    size_t out  = 0;
    for (size_t i = 0; i < envc; i++) {
        const char *entry = environ[i];
        if (_env_entry_has_key(entry, DICE_PLUGIN_MODULES) ||
            _env_entry_has_key(entry, LD_PRELOAD_ENV) ||
            _env_entry_has_key(entry, LOTTO_CLI_PRELOAD)) {
            continue;
        }
        envp[out++] = (char *)entry;
    }

    char runtime_entry[MAX_LIST_STR + 32];
    if (runtime_modules != NULL && runtime_modules[0] != '\0') {
        sys_snprintf(runtime_entry, sizeof(runtime_entry), "%s=%s",
                     DICE_PLUGIN_MODULES, runtime_modules);
        envp[out++] = runtime_entry;
    }
    envp[out] = NULL;

    int res = execvpe(argv[0], argv, envp);
    sys_free(envp);
    return res;
}

static void
_derive_default_paths(const args_t *args, char *qemu_bin, char *module_dir,
                      char *module_fallback_dir)
{
    char resolved_path[PATH_MAX];
    _resolve_lotto_anchor(args, resolved_path);

    char lotto_dir_buf[PATH_MAX];
    sys_sprintf(lotto_dir_buf, "%s", dirname(resolved_path));
    char lotto_dir_copy[PATH_MAX];
    sys_sprintf(lotto_dir_copy, "%s", lotto_dir_buf);
    char prefix_dir_buf[PATH_MAX];
    sys_sprintf(prefix_dir_buf, "%s", dirname(lotto_dir_copy));

    char candidate_qemu_build[PATH_MAX];
    char candidate_qemu_install[PATH_MAX];
    sys_sprintf(candidate_qemu_build, "%s/bin/qemu-system-aarch64",
                lotto_dir_buf);
    sys_sprintf(candidate_qemu_install, "%s/bin/qemu-system-aarch64",
                prefix_dir_buf);

    const char *selected_qemu =
        _existing_path(candidate_qemu_build, candidate_qemu_install);
    sys_sprintf(qemu_bin, "%s", selected_qemu);

    sys_sprintf(module_dir, "%s/modules", lotto_dir_buf);
    sys_sprintf(module_fallback_dir, "%s/share/lotto/modules", prefix_dir_buf);
}

static const char *
_lotto_runtime_dso_path(const args_t *args, bool verbose_enabled)
{
    static char selected[4][PATH_MAX];
    static unsigned int idx;

    char resolved_path[PATH_MAX];
    _resolve_lotto_anchor(args, resolved_path);

    char lotto_dir_buf[PATH_MAX];
    sys_sprintf(lotto_dir_buf, "%s", dirname(resolved_path));
    char lotto_dir_copy[PATH_MAX];
    sys_sprintf(lotto_dir_copy, "%s", lotto_dir_buf);
    char prefix_dir_buf[PATH_MAX];
    sys_sprintf(prefix_dir_buf, "%s", dirname(lotto_dir_copy));

    const char *libname =
        verbose_enabled ? "liblotto-runtime-dbg.so" : "liblotto-runtime.so";
    char preferred[PATH_MAX];
    char fallback[PATH_MAX];
    sys_sprintf(preferred, "%s/%s", lotto_dir_buf, libname);
    sys_sprintf(fallback, "%s/lib/%s", prefix_dir_buf, libname);

    idx              = (idx + 1U) % 4U;
    const char *path = _existing_file(preferred, fallback);
    sys_sprintf(selected[idx], "%s", path);
    return selected[idx];
}

static const char *
_select_qemu_plugin(const args_t *args, const char *module_dir,
                    const char *module_fallback_dir, bool verbose_enabled)
{
    const char *runtime_qemu_plugin =
        verbose_enabled ? "lotto-runtime-dbg-qemu.so" : "lotto-runtime-qemu.so";
    const char *plugin =
        _plugin_path(module_dir, module_fallback_dir, runtime_qemu_plugin);
    if (access(plugin, R_OK) == 0) {
        return plugin;
    }
    return _lotto_runtime_dso_path(args, verbose_enabled);
}

static bool
_is_runtime_qemu_plugin_path(const char *path)
{
    const char *end;
    char base_buf[PATH_MAX];
    size_t len;

    if (path == NULL || path[0] == '\0') {
        return false;
    }

    end = strchr(path, ',');
    if (end == NULL) {
        end = path + sys_strlen(path);
    }
    len = (size_t)(end - path);
    ASSERT(len < sizeof(base_buf));
    sys_memcpy(base_buf, path, len);
    base_buf[len] = '\0';

    const char *base = strrchr(base_buf, '/');
    base             = base != NULL ? base + 1 : base_buf;
    return sys_strcmp(base, "lotto-runtime-qemu.so") == 0 ||
           sys_strcmp(base, "lotto-runtime-dbg-qemu.so") == 0 ||
           sys_strcmp(base, "liblotto-runtime.so") == 0 ||
           sys_strcmp(base, "liblotto-runtime-dbg.so") == 0;
}

static const char *
_qemu_plugin_spec(const char *plugin_path, const flags_t *flags)
{
    static char selected[4][MAX_LIST_STR];
    static unsigned int idx;
    int written;
    (void)flags;

    idx = (idx + 1U) % 4U;
    written =
        sys_snprintf(selected[idx], sizeof(selected[idx]), "%s", plugin_path);
    ASSERT(written >= 0 && (size_t)written < sizeof(selected[idx]));

    return selected[idx];
}

static bool
_qemu_plugins_enabled(const flags_t *flags)
{
    const char *mode = flags_get_sval(flags, FLAG_QEMU_PLUGINS);
    if (mode == NULL || mode[0] == '\0') {
        return true;
    }

    return sys_strcmp(mode, "off") != 0 && sys_strcmp(mode, "false") != 0 &&
           sys_strcmp(mode, "no") != 0 && sys_strcmp(mode, "0") != 0;
}

void
qemu_resolve_replay_args(args_t *args, const flags_t *flags)
{
    if (args == NULL || args->argv == NULL || args->argc < 2) {
        return;
    }

    bool has_qemu_plugin = false;
    for (int i = 0; i + 1 < args->argc; i++) {
        if (sys_strcmp(args->argv[i], "-plugin") == 0 &&
            _is_runtime_qemu_plugin_path(args->argv[i + 1])) {
            has_qemu_plugin = true;
            break;
        }
    }
    if (!has_qemu_plugin) {
        return;
    }

    const bool verbose_enabled = flag_verbose_enabled(flags);
    char default_qemu_bin[PATH_MAX];
    char default_module_dir[PATH_MAX];
    char fallback_module_dir[PATH_MAX];
    _derive_default_paths(args, default_qemu_bin, default_module_dir,
                          fallback_module_dir);
    const char *selected = _qemu_plugin_spec(
        _select_qemu_plugin(args, default_module_dir, fallback_module_dir,
                            verbose_enabled),
        flags);

    for (int i = 0; i + 1 < args->argc; i++) {
        if (sys_strcmp(args->argv[i], "-plugin") == 0 &&
            _is_runtime_qemu_plugin_path(args->argv[i + 1])) {
            args->argv[i + 1] = (char *)selected;
            return;
        }
    }
}

void
qemu_prefix_args(args_t *args, const flags_t *flags)
{
    if (!flags_is_on(flags, flag_stress_qemu())) {
        return;
    }

    const bool verbose_enabled = flag_verbose_enabled(flags);
    const bool plugins_enabled = _qemu_plugins_enabled(flags);
    const char *mem            = flags_get_sval(flags, FLAG_QEMU_MEM);
    const char *cpu            = flags_get_sval(flags, FLAG_QEMU_CPU);
    if (mem == NULL || mem[0] == '\0') {
        mem = sys_getenv("MEM");
    }
    if (cpu == NULL || cpu[0] == '\0') {
        cpu = sys_getenv("CPU");
    }
    if (mem == NULL || mem[0] == '\0') {
        mem = "200M";
    }
    if (cpu == NULL || cpu[0] == '\0') {
        cpu = "4";
    }

    char default_qemu_bin[PATH_MAX];
    char default_module_dir[PATH_MAX];
    char fallback_module_dir[PATH_MAX];
    _derive_default_paths(args, default_qemu_bin, default_module_dir,
                          fallback_module_dir);

    const char *default_qemu_args[] = {
        "-machine",   "virt", "-m", mem,          "-cpu", "cortex-a57",
        "-nographic", "-smp", cpu,  "-no-reboot", NULL};
    int default_argc = 0;
    while (default_qemu_args[default_argc] != NULL) {
        default_argc++;
    }

    const int plugin_argc = plugins_enabled ? 2 : 0;
    const char *plugin    = NULL;
    if (plugins_enabled) {
        plugin = _qemu_plugin_spec(_select_qemu_plugin(args, default_module_dir,
                                                       fallback_module_dir,
                                                       verbose_enabled),
                                   flags);
    }

    char **argv =
        sys_calloc((size_t)(1 + plugin_argc + default_argc + args->argc + 1),
                   sizeof(char *));
    int i     = 0;
    argv[i++] = default_qemu_bin;
    if (plugins_enabled) {
        argv[i++] = "-plugin";
        argv[i++] = (char *)plugin;
    }
    for (int j = 0; j < default_argc; j++) {
        argv[i++] = (char *)default_qemu_args[j];
    }
    for (int j = 0; j < args->argc; j++) {
        argv[i++] = args->argv[j];
    }
    argv[i]    = NULL;
    args->argv = argv;
    args->argc = i;
}

void
qemu_enable_stress_prefix(bool enabled)
{
    execute_set_command_prefix(enabled ? qemu_prefix_args : NULL);
}

int
qemu(args_t *args, flags_t *flags)
{
    char **reexec_argv = sys_calloc((size_t)(args->argc + 3), sizeof(char *));
    int r              = 0;
    reexec_argv[r++]   = args->arg0;
    reexec_argv[r++]   = "stress";
    reexec_argv[r++]   = "-Q";

    bool skipped_subcmd = false;
    for (int i = 0; i < args->argc; i++) {
        if (!skipped_subcmd && sys_strcmp(args->argv[i], "qemu") == 0) {
            skipped_subcmd = true;
            continue;
        }
        reexec_argv[r++] = args->argv[i];
    }
    reexec_argv[r] = NULL;

    if (flag_verbose_enabled(flags)) {
        sys_fprintf(stdout, "[lotto] re-exec:");
        for (int i = 0; reexec_argv[i] != NULL; i++) {
            sys_fprintf(stdout, " %s", reexec_argv[i]);
        }
        sys_fprintf(stdout, "\n");
    }

    int reexec_res = execvpe(reexec_argv[0], reexec_argv, environ);
    if (reexec_res != 0) {
        sys_fprintf(stderr, "error: failed to re-exec '%s stress -Q'\n",
                    reexec_argv[0]);
    }
    sys_free(reexec_argv);
    return reexec_res;
}

ON_DRIVER_REGISTER_COMMANDS({
    execute_set_replay_args_resolver(qemu_resolve_replay_args);
    flag_t sel[] = {flag_verbose(),
                    FLAG_QEMU_BIN,
                    FLAG_QEMU_PLUGIN_DEBUG,
                    FLAG_QEMU_PLUGINS,
                    FLAG_QEMU_MEM,
                    FLAG_QEMU_CPU,
                    FLAG_QEMU_NO_COMMON_ARGS,
                    0};
    subcmd_register(qemu, "qemu", "[<options>] [--] [qemu_args...]",
                    "Shortcut for 'stress -Q -- [qemu_args...]'", true, sel,
                    flags_default, SUBCMD_GROUP_OTHER);
})
