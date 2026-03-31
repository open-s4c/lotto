#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/cmake_variables.h>
#include <lotto/driver/main.h>
#include <lotto/sys/modules.h>
#include <sys/personality.h>

#define LOTTO_BOOTSTRAP_ENV "LOTTO_DRIVER_BOOTSTRAPPED"
#define DICE_PLUGIN_MODULES "DICE_PLUGIN_MODULES"
#define LOTTO_CLI_PRELOAD   "LOTTO_CLI_PRELOAD"
#define LOTTO_LOAD_RUNTIME  "LOTTO_LOAD_RUNTIME"
#define MAX_LIST_STR        ((size_t)(32 * 1024))

typedef struct preload_state_s {
    char *buf;
    size_t len;
    size_t cap;
} preload_state_t;

typedef struct driver_options_s {
    const char *module_dir;
    const char *module_list;
    char runtime_loads[MAX_LIST_STR];
    char driver_loads[MAX_LIST_STR];
} driver_options_t;

static void driver_options(int argc, char **argv, driver_options_t *opts);
static int add_driver_module(module_t *module, void *arg);
static void prepend_env_path(const char *name, const char *value);
static void append_preload(preload_state_t *state, const char *path);
static void append_preload_list(preload_state_t *state, const char *paths);
static void append_option_value(char *buf, size_t cap, const char *value);
static bool path_is_readable(const char *path);
static bool path_printf(char *buf, size_t cap, const char *fmt,
                        const char *value);
static int exec_self(char **argv);

int
main(int argc, char **argv)
{
    const int old_personality = personality(ADDR_NO_RANDOMIZE);
    if (!(old_personality & ADDR_NO_RANDOMIZE)) {
        personality(ADDR_NO_RANDOMIZE);
        return exec_self(argv);
    }

    if (getenv(LOTTO_BOOTSTRAP_ENV) == NULL) {
        driver_options_t opts = {0};
        char resolved_path[PATH_MAX];
        char driver_path[PATH_MAX];
        char binary_dir[PATH_MAX];
        char lib_dir[PATH_MAX];

        driver_options(argc, argv, &opts);
        lotto_module_scan(LOTTO_MODULE_BUILD_DIR, LOTTO_MODULE_INSTALL_DIR,
                          opts.module_dir);
        if (lotto_module_enable_only(opts.module_list) != 0) {
            return 1;
        }

        if (realpath(argv[0], resolved_path) == NULL) {
            if (argv[0][0] == '/') {
                snprintf(resolved_path, sizeof(resolved_path), "%s", argv[0]);
            } else {
                perror("failed to resolve lotto path");
                return 1;
            }
        }

        if (!path_printf(binary_dir, sizeof(binary_dir), "%s",
                         dirname(resolved_path))) {
            fprintf(stderr, "lotto path too long\n");
            return 1;
        }
        if (!path_printf(driver_path, sizeof(driver_path),
                         "%s/liblotto-driver.so", binary_dir)) {
            fprintf(stderr, "driver path too long\n");
            return 1;
        }
        if (!path_is_readable(driver_path)) {
            if (!path_printf(lib_dir, sizeof(lib_dir), "%s/../lib",
                             binary_dir)) {
                fprintf(stderr, "library path too long\n");
                return 1;
            }
            if (!path_printf(driver_path, sizeof(driver_path),
                             "%s/liblotto-driver.so", lib_dir)) {
                fprintf(stderr, "driver path too long\n");
                return 1;
            }
        } else {
            if (!path_printf(lib_dir, sizeof(lib_dir), "%s", binary_dir)) {
                fprintf(stderr, "library path too long\n");
                return 1;
            }
        }

        preload_state_t preload        = {.buf = malloc(MAX_LIST_STR),
                                          .len = 0,
                                          .cap = MAX_LIST_STR};
        preload_state_t plugin_modules = {.buf = malloc(MAX_LIST_STR),
                                          .len = 0,
                                          .cap = MAX_LIST_STR};
        if (preload.buf == NULL || plugin_modules.buf == NULL) {
            perror("malloc");
            free(preload.buf);
            free(plugin_modules.buf);
            return 1;
        }
        preload.buf[0]        = '\0';
        plugin_modules.buf[0] = '\0';

        append_preload(&preload, driver_path);
        if (lotto_module_foreach(add_driver_module, &preload) != 0) {
            free(preload.buf);
            free(plugin_modules.buf);
            return 1;
        }
        if (lotto_module_foreach(add_driver_module, &plugin_modules) != 0) {
            free(preload.buf);
            free(plugin_modules.buf);
            return 1;
        }
        if (getenv("LD_PRELOAD") != NULL && getenv("LD_PRELOAD")[0] != '\0') {
            setenv(LOTTO_CLI_PRELOAD, getenv("LD_PRELOAD"), true);
            append_preload(&preload, getenv("LD_PRELOAD"));
        } else {
            unsetenv(LOTTO_CLI_PRELOAD);
        }
        append_preload_list(&preload, opts.driver_loads);

        if (opts.runtime_loads[0] != '\0') {
            setenv(LOTTO_LOAD_RUNTIME, opts.runtime_loads, true);
        } else {
            unsetenv(LOTTO_LOAD_RUNTIME);
        }

        setenv("LD_PRELOAD", preload.buf, true);
        setenv(DICE_PLUGIN_MODULES, plugin_modules.buf, true);
        setenv(LOTTO_BOOTSTRAP_ENV, "1", true);
        prepend_env_path("LD_LIBRARY_PATH", lib_dir);
        free(preload.buf);
        free(plugin_modules.buf);
        return exec_self(argv);
    }
    unsetenv(LOTTO_BOOTSTRAP_ENV);

    return driver_main(argc, argv);
}

static void
driver_options(int argc, char **argv, driver_options_t *opts)
{
    opts->module_dir       = NULL;
    opts->module_list      = NULL;
    opts->runtime_loads[0] = '\0';
    opts->driver_loads[0]  = '\0';

    for (int argv_idx = 1; argv_idx + 1 < argc; argv_idx += 2) {
        if (strcmp(argv[argv_idx], "--plugin-dir") == 0) {
            opts->module_dir = argv[argv_idx + 1];
            continue;
        }
        if (strcmp(argv[argv_idx], "--plugins") == 0) {
            opts->module_list = argv[argv_idx + 1];
            continue;
        }
        if (strcmp(argv[argv_idx], "--load-runtime") == 0) {
            append_option_value(opts->runtime_loads,
                                sizeof(opts->runtime_loads),
                                argv[argv_idx + 1]);
            continue;
        }
        if (strcmp(argv[argv_idx], "--load-driver") == 0) {
            append_option_value(opts->driver_loads, sizeof(opts->driver_loads),
                                argv[argv_idx + 1]);
            continue;
        }
        break;
    }
}

static bool
path_printf(char *buf, size_t cap, const char *fmt, const char *value)
{
    int written = snprintf(buf, cap, fmt, value);
    return written >= 0 && (size_t)written < cap;
}

static int
add_driver_module(module_t *module, void *arg)
{
    preload_state_t *state = arg;

    if (!(module->kind & MODULE_KIND_CLI)) {
        return 0;
    }
    append_preload(state, module->path);
    return 0;
}

static void
prepend_env_path(const char *name, const char *value)
{
    char buf[MAX_LIST_STR];
    const char *cur = getenv(name);

    if (cur != NULL && cur[0] != '\0') {
        snprintf(buf, sizeof(buf), "%s:%s", value, cur);
    } else {
        snprintf(buf, sizeof(buf), "%s", value);
    }
    setenv(name, buf, true);
}

static bool
path_is_readable(const char *path)
{
    return access(path, R_OK) == 0;
}

static void
append_preload(preload_state_t *state, const char *path)
{
    int written;

    if (state->len > 0) {
        written = snprintf(state->buf + state->len, state->cap - state->len,
                           ":%s", path);
    } else {
        written = snprintf(state->buf + state->len, state->cap - state->len,
                           "%s", path);
    }

    if (written < 0 || (size_t)written >= state->cap - state->len) {
        fprintf(stderr, "error: LD_PRELOAD list too long\n");
        exit(1);
    }
    state->len += (size_t)written;
}

static void
append_preload_list(preload_state_t *state, const char *paths)
{
    char paths_copy[MAX_LIST_STR];

    if (paths == NULL || paths[0] == '\0') {
        return;
    }

    snprintf(paths_copy, sizeof(paths_copy), "%s", paths);
    for (char *path = strtok(paths_copy, ":"); path != NULL;
         path       = strtok(NULL, ":")) {
        append_preload(state, path);
    }
}

static void
append_option_value(char *buf, size_t cap, const char *value)
{
    int written;
    size_t len = strlen(buf);

    if (value == NULL || value[0] == '\0') {
        return;
    }

    if (len > 0) {
        written = snprintf(buf + len, cap - len, ":%s", value);
    } else {
        written = snprintf(buf + len, cap - len, "%s", value);
    }

    if (written < 0 || (size_t)written >= cap - len) {
        fprintf(stderr, "error: option list too long\n");
        exit(1);
    }
}

static int
exec_self(char **argv)
{
#ifdef __linux__
    execv("/proc/self/exe", argv);
#else
    char *abs_path   = realpath(argv[0], NULL);
    const char *path = abs_path ? abs_path : argv[0];
    execv(path, argv);
    free(abs_path);
#endif
    perror("failed to exec");
    return 1;
}
