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
#define MAX_COMMAND_ARGS    2
#define MAX_LIST_STR        ((size_t)(32 * 1024))

typedef struct preload_state_s {
    char *buf;
    size_t len;
    size_t cap;
} preload_state_t;

static void driver_options(int argc, char **argv, const char **module_dir,
                           const char **module_list);
static int add_driver_module(module_t *module, void *arg);
static void prepend_env_path(const char *name, const char *value);
static bool join_path(char *dst, size_t dst_size, const char *left,
                      const char *right);
static void append_preload(preload_state_t *state, const char *path);
static bool path_is_readable(const char *path);
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
        const char *module_dir  = NULL;
        const char *module_list = NULL;
        char resolved_path[PATH_MAX];
        char driver_path[PATH_MAX];
        char binary_dir[PATH_MAX];
        char lib_dir[PATH_MAX];

        driver_options(argc, argv, &module_dir, &module_list);
        lotto_module_scan(LOTTO_MODULE_BUILD_DIR, LOTTO_MODULE_INSTALL_DIR,
                          module_dir);
        if (lotto_module_enable_only(module_list) != 0) {
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

        snprintf(binary_dir, sizeof(binary_dir), "%s", dirname(resolved_path));
        if (!join_path(driver_path, sizeof(driver_path), binary_dir,
                       "/liblotto-driver.so")) {
            fprintf(stderr, "error: lotto driver path too long\n");
            return 1;
        }
        if (!path_is_readable(driver_path)) {
            if (!join_path(lib_dir, sizeof(lib_dir), binary_dir, "/../lib")) {
                fprintf(stderr, "error: lotto library path too long\n");
                return 1;
            }
            if (!join_path(driver_path, sizeof(driver_path), lib_dir,
                           "/liblotto-driver.so")) {
                fprintf(stderr, "error: lotto driver path too long\n");
                return 1;
            }
        } else {
            snprintf(lib_dir, sizeof(lib_dir), "%s", binary_dir);
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
driver_options(int argc, char **argv, const char **module_dir,
               const char **module_list)
{
    *module_dir  = NULL;
    *module_list = NULL;

    for (int argv_idx = 1; argv_idx <= MAX_COMMAND_ARGS * 2 - 1;
         argv_idx += 2) {
        if (argc > argv_idx + 1) {
            if (strcmp(argv[argv_idx], "--plugin-dir") == 0) {
                *module_dir = argv[argv_idx + 1];
            }
            if (strcmp(argv[argv_idx], "--plugins") == 0) {
                *module_list = argv[argv_idx + 1];
            }
        }
    }
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
join_path(char *dst, size_t dst_size, const char *left, const char *right)
{
    size_t left_len;
    size_t right_len;

    if (dst == NULL || dst_size == 0 || left == NULL || right == NULL) {
        return false;
    }

    left_len  = strlen(left);
    right_len = strlen(right);
    if (left_len + right_len + 1 > dst_size) {
        return false;
    }

    memcpy(dst, left, left_len);
    memcpy(dst + left_len, right, right_len + 1);
    return true;
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
