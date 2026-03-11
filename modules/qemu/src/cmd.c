#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/driver/flagmgr.h>
#include <lotto/driver/subcmd.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

DECLARE_FLAG_VERBOSE;
DECLARE_COMMAND_FLAG(QEMU_BIN, "", "qemu-bin", "PATH",
                     "path to qemu-system-aarch64", flag_sval(""))
DECLARE_COMMAND_FLAG(QEMU_PLUGIN_DIR, "", "qemu-plugin-dir", "DIR",
                     "directory containing qlotto qemu plugins", flag_sval(""))
DECLARE_COMMAND_FLAG(QEMU_WITH_GDB_SERVER, "", "gdb-server", "",
                     "load qlotto gdb server plugin", flag_off())
DECLARE_COMMAND_FLAG(QEMU_WITH_MEASURE, "", "measure", "",
                     "load qlotto measure plugin", flag_off())
DECLARE_COMMAND_FLAG(QEMU_WITH_SNAPSHOT, "", "snapshot", "",
                     "load qlotto snapshot plugin", flag_off())
DECLARE_COMMAND_FLAG(QEMU_PLUGIN_DEBUG, "", "plugin-debug", "",
                     "append '-d plugin' for qemu plugin debugging", flag_off())
DECLARE_COMMAND_FLAG(QEMU_MEM, "", "qemu-mem", "SIZE",
                     "default qemu memory size for injected args",
                     flag_sval(""))
DECLARE_COMMAND_FLAG(QEMU_CPU, "", "qemu-cpu", "N",
                     "default qemu cpu count for injected args", flag_sval(""))
DECLARE_COMMAND_FLAG(QEMU_NO_COMMON_ARGS, "", "no-common-arguments", "",
                     "do not inject common QEMU args", flag_off())

extern char **environ;

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
_derive_default_paths(const args_t *args, char *qemu_bin, char *plugin_dir,
                      char *plugin_fallback_dir)
{
    char resolved_path[PATH_MAX];
    if (realpath(args->arg0, resolved_path) == NULL) {
        sys_sprintf(resolved_path, "%s", args->arg0);
    }

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

    sys_sprintf(plugin_dir, "%s/qlotto/modules", lotto_dir_buf);
    sys_sprintf(plugin_fallback_dir, "%s/share/lotto/qlotto/modules",
                prefix_dir_buf);
}

int
qemu(args_t *args, flags_t *flags)
{
    const bool with_gdb_server = flags_is_on(flags, FLAG_QEMU_WITH_GDB_SERVER);
    const bool with_measure    = flags_is_on(flags, FLAG_QEMU_WITH_MEASURE);
    const bool with_snapshot   = flags_is_on(flags, FLAG_QEMU_WITH_SNAPSHOT);
    const bool plugin_debug    = flags_is_on(flags, FLAG_QEMU_PLUGIN_DEBUG);
    const bool with_defaults   = !flags_is_on(flags, FLAG_QEMU_NO_COMMON_ARGS);
    const char *mem            = flags_get_sval(flags, FLAG_QEMU_MEM);
    if (mem == NULL || mem[0] == '\0') {
        mem = getenv("MEM");
    }
    if (mem == NULL || mem[0] == '\0') {
        mem = "200M";
    }
    const char *cpu = flags_get_sval(flags, FLAG_QEMU_CPU);
    if (cpu == NULL || cpu[0] == '\0') {
        cpu = getenv("CPU");
    }
    if (cpu == NULL || cpu[0] == '\0') {
        cpu = "2";
    }

    char default_qemu_bin[PATH_MAX];
    char default_plugin_dir[PATH_MAX];
    char fallback_plugin_dir[PATH_MAX];
    _derive_default_paths(args, default_qemu_bin, default_plugin_dir,
                          fallback_plugin_dir);

    const char *qemu_bin = flags_get_sval(flags, FLAG_QEMU_BIN);
    if (qemu_bin[0] == '\0') {
        qemu_bin = default_qemu_bin;
    }

    const char *plugin_dir = flags_get_sval(flags, FLAG_QEMU_PLUGIN_DIR);
    if (plugin_dir[0] == '\0') {
        plugin_dir = default_plugin_dir;
    }

    const char *plugins[4];
    int nplugins = 0;

    plugins[nplugins++] =
        _plugin_path(plugin_dir, fallback_plugin_dir, "libplugin-lotto.so");
    if (with_gdb_server) {
        plugins[nplugins++] = _plugin_path(plugin_dir, fallback_plugin_dir,
                                           "libplugin-gdb-server.so");
    }
    if (with_measure) {
        plugins[nplugins++] = _plugin_path(plugin_dir, fallback_plugin_dir,
                                           "libplugin-measure.so");
    }
    if (with_snapshot) {
        plugins[nplugins++] = _plugin_path(plugin_dir, fallback_plugin_dir,
                                           "libplugin-snapshot.so");
    }

    const char *default_qemu_args[] = {
        "-machine",   "virt", "-m", mem,          "-cpu", "cortex-a57",
        "-nographic", "-smp", cpu,  "-no-reboot", NULL,
    };
    int default_argc = 0;
    if (with_defaults) {
        while (default_qemu_args[default_argc] != NULL) {
            default_argc++;
        }
    }

    const int fixed_args =
        1 + nplugins * 2 + (plugin_debug ? 2 : 0) + default_argc;
    char **argv =
        sys_calloc((size_t)(fixed_args + args->argc + 1), sizeof(char *));
    int i = 0;

    argv[i++] = (char *)qemu_bin;
    for (int p = 0; p < nplugins; p++) {
        argv[i++] = "-plugin";
        argv[i++] = (char *)plugins[p];
    }
    if (plugin_debug) {
        argv[i++] = "-d";
        argv[i++] = "plugin";
    }
    for (int j = 0; j < default_argc; j++) {
        argv[i++] = (char *)default_qemu_args[j];
    }

    bool skipped_subcmd = false;
    for (int j = 0; j < args->argc; j++) {
        if (!skipped_subcmd && sys_strcmp(args->argv[j], "qemu") == 0) {
            skipped_subcmd = true;
            continue;
        }
        argv[i++] = args->argv[j];
    }
    argv[i] = NULL;

    if (flags_is_on(flags, FLAG_VERBOSE)) {
        sys_fprintf(stdout, "[lotto] exec:");
        for (int j = 0; argv[j] != NULL; j++) {
            sys_fprintf(stdout, " %s", argv[j]);
        }
        sys_fprintf(stdout, "\n");
    }

    int res = execvpe(argv[0], argv, environ);
    if (res != 0) {
        sys_fprintf(stderr, "error: failed to run qemu command '%s'\n",
                    argv[0]);
    }

    sys_free(argv);
    return res;
}

LOTTO_ON_DRIVER_INIT({
    flag_t sel[] = {FLAG_VERBOSE,
                    FLAG_QEMU_BIN,
                    FLAG_QEMU_PLUGIN_DIR,
                    FLAG_QEMU_WITH_GDB_SERVER,
                    FLAG_QEMU_WITH_MEASURE,
                    FLAG_QEMU_WITH_SNAPSHOT,
                    FLAG_QEMU_PLUGIN_DEBUG,
                    FLAG_QEMU_MEM,
                    FLAG_QEMU_CPU,
                    FLAG_QEMU_NO_COMMON_ARGS,
                    0};
    subcmd_register(qemu, "qemu", "[<options>] [--] [qemu_args...]",
                    "Run Lotto-built qemu-system-aarch64 with qlotto plugins",
                    true, sel, flags_default, SUBCMD_GROUP_OTHER);
})
