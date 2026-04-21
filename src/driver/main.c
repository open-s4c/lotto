#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/cmake_variables.h>
#include <lotto/driver/args.h>
#include <lotto/driver/exec_info.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/driver/main.h>
#include <lotto/driver/preload.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/utils.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/modules.h>
#include <lotto/sys/stdio.h>

#define LOTTO_LOAD_RUNTIME "LOTTO_LOAD_RUNTIME"
#define MAX_LIST_STR       ((size_t)(32 * 1024))

typedef struct driver_options_s {
    const char *module_dir;
    const char *module_list;
    char runtime_loads[MAX_LIST_STR];
    char driver_loads[MAX_LIST_STR];
    int subcmd_pos;
} driver_options_t;

static void append_option_value(char *buf, size_t cap, const char *value);

static void
driver_options(int argc, char **argv, driver_options_t *opts)
{
    opts->module_dir       = NULL;
    opts->module_list      = NULL;
    opts->runtime_loads[0] = '\0';
    opts->driver_loads[0]  = '\0';
    opts->subcmd_pos       = 0;

    for (int argv_idx = 1; argv_idx + 1 < argc; argv_idx += 2) {
        if (strcmp(argv[argv_idx], "--plugin-dir") == 0) {
            opts->module_dir = argv[argv_idx + 1];
            opts->subcmd_pos = argv_idx + 1;
            continue;
        }
        if (strcmp(argv[argv_idx], "--plugins") == 0) {
            opts->module_list = argv[argv_idx + 1];
            opts->subcmd_pos  = argv_idx + 1;
            continue;
        }
        if (strcmp(argv[argv_idx], "--load-runtime") == 0) {
            append_option_value(opts->runtime_loads,
                                sizeof(opts->runtime_loads),
                                argv[argv_idx + 1]);
            opts->subcmd_pos = argv_idx + 1;
            continue;
        }
        if (strcmp(argv[argv_idx], "--load-driver") == 0) {
            append_option_value(opts->driver_loads, sizeof(opts->driver_loads),
                                argv[argv_idx + 1]);
            opts->subcmd_pos = argv_idx + 1;
            continue;
        }
        break;
    }
}

static void
describe(subcmd_t *scmd)
{
    sys_fprintf(stdout, "Description:\n    %s\n\n", scmd->desc);
    sys_fprintf(stdout, "Usage:\n");
    sys_fprintf(
        stdout,
        "    lotto [--plugin-dir DIR] [--plugins P1[,P2]] "
        "[--load-runtime PATH[:PATH...]] [--load-driver PATH[:PATH...]] "
        "%s [<options>] %s\n\n",
        scmd->name, scmd->args);
    flags_help(scmd->defaults(), scmd->runtime_sel, scmd->sel);
}

int
driver_main(int argc, char **argv)
{
    driver_options_t opts = {0};
    char *arg0            = argv[0];

    sys_setvbuf(stdout, NULL, _IONBF, 0);
    sys_setvbuf(stderr, NULL, _IONBF, 0);

    driver_options(argc, argv, &opts);
    if (opts.runtime_loads[0] != '\0') {
        setenv(LOTTO_LOAD_RUNTIME, opts.runtime_loads, true);
    }

    lotto_module_scan(LOTTO_MODULE_BUILD_DIR, LOTTO_MODULE_INSTALL_DIR,
                      opts.module_dir);
    if (0 != lotto_module_enable_only(opts.module_list))
        return 1;

#if defined(LOTTO_EMBED_LIB) && LOTTO_EMBED_LIB == 0
    {
        char *exe_path = getenv("LOTTO_EXECUTABLE_PATH");
        char resolved_path[PATH_MAX];
        if (!exe_path) {
            exe_path = realpath(argv[0], resolved_path);
        }
        ASSERT(exe_path != 0 &&
            "Unable to resolve path. You must provide lotto's path or a "
            "real path alias.");
        preload_set_libpath(dirname(exe_path));
    }
#endif

    const char *scmd_str = argv[opts.subcmd_pos + 1];
    subcmd_t *scmd       = subcmd_find(scmd_str != NULL ? scmd_str : "-h");
    if (scmd == NULL) {
        if (scmd_str != NULL) {
            subcmd_t *scmd_closest = subcmd_find_closest(scmd_str);
            if (scmd_closest != NULL) {
                sys_fprintf(stderr, "error: Did you mean %s ?\n",
                            scmd_closest->name);
            }
        }
        sys_fprintf(stderr, "error: unknown command %s\n", scmd_str);
        return 1;
    }

    exec_info_t *exec_info = get_exec_info();
    exec_info->hash_actual = get_lotto_hash(arg0);
    exec_info->args =
        (scmd->group != SUBCMD_GROUP_OTHER) ?
            ARGS(argc - opts.subcmd_pos - 1, argv + opts.subcmd_pos + 1) :
            ARGS(argc - opts.subcmd_pos, argv + opts.subcmd_pos);

    exec_info->args.arg0 = arg0;
        flags_t *flags       = scmd->defaults();
    switch (
        flags_parse(flags, &exec_info->args, scmd->runtime_sel, scmd->sel)) {
        case FLAGS_PARSE_OK: {
            if (validate_module_enable_flags(flags) != 0) {
                return 1;
            }
            if (scmd->group == SUBCMD_GROUP_RUN && exec_info->args.argc < 1) {
                sys_fprintf(stderr, "error: no program to run specified\n");
                return 1;
            }
            if (scmd->group == SUBCMD_GROUP_TRACE && exec_info->args.argc > 0) {
                sys_fprintf(stderr,
                            "error: this command does not expect arguments\n");
                return 1;
            }

            int r = scmd->func(&exec_info->args, flags);
            if (r == -1 && scmd->group == SUBCMD_GROUP_RUN) {
                sys_fprintf(stderr, "error: could not run the program %s\n",
                            exec_info->args.argv[0]);
            }
            return r;
        }
        case FLAGS_PARSE_HELP:
            if (scmd->name[0] != '-')
                describe(scmd);
            else
                scmd->func(&exec_info->args, flags);
            return -1;
        case FLAGS_PARSE_LIST_FLAGS:
            flags_list(scmd->runtime_sel, scmd->sel);
            return 0;
        case FLAGS_PARSE_ERROR:
            return -1;
        default:
            ASSERT(0 && "should never reach here");
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

    ASSERT(written >= 0 && (size_t)written < cap - len);
}
