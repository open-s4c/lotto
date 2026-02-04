#include <dlfcn.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/cli/args.h>
#include <lotto/cli/exec_info.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/utils.h>
#include <lotto/cmake_variables.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/plugin.h>
#include <lotto/sys/stdio.h>
#include <sys/personality.h>

#define MAX_COMMAND_ARGS 2

static int _load_plugin(plugin_t *plugin, void *arg);

static void
describe(subcmd_t *scmd)
{
    sys_fprintf(stdout, "Description:\n    %s\n\n", scmd->desc);
    sys_fprintf(stdout, "Usage:\n");
    sys_fprintf(stdout, "    lotto [--plugin-dir DIR] %s [<options>] %s\n\n",
                scmd->name, scmd->args);
    flags_help(scmd->defaults(), scmd->runtime_sel, scmd->sel);
}

int
main(int argc, char **argv)
{
    const int old_personality = personality(ADDR_NO_RANDOMIZE);
    if (!(old_personality & ADDR_NO_RANDOMIZE)) {
        personality(ADDR_NO_RANDOMIZE);
    }

    const char *plugin_dir  = NULL;
    const char *plugin_list = NULL;
    int subcmd_pos          = 0;
    char *arg0              = argv[0];

    // We must process plugin dir before we load any CLI plugins, that's why we
    // cannot use SUBCMD_GROUP_OTHER to simply collect possible plugin-related
    // flags
    // Check up to the first MAX_COMMAND_ARGS argument-value pairs
    int max_command_args_idx = MAX_COMMAND_ARGS * 2 - 1;
    for (int argv_idx = 1; argv_idx <= max_command_args_idx; argv_idx += 2) {
        if (argc > argv_idx + 2) {
            if (strcmp(argv[argv_idx], "--plugin-dir") == 0) {
                plugin_dir = argv[argv_idx + 1];
                // subcmd array index - 1
                subcmd_pos = argv_idx + 1;
            }
            if (strcmp(argv[argv_idx], "--plugins") == 0) {
                plugin_list = argv[argv_idx + 1];
                // subcmd array index - 1
                subcmd_pos = argv_idx + 1;
            }
        }
    }

    if (getenv("LOTTO_CLI_RESTARTED")) {
        const char* added_preload_len_str = getenv("LOTTO_ADDED_PRELOAD_LEN");
        if (added_preload_len_str && strcmp(added_preload_len_str, "0") != 0) {
            char* ld_preload = getenv("LD_PRELOAD") + (size_t) atoll(added_preload_len_str);
            setenv("LD_PRELOAD", ld_preload, true);
        }
    } else {
        lotto_plugin_scan(LOTTO_MODULE_BUILD_DIR, LOTTO_MODULE_INSTALL_DIR,
                          plugin_dir);
        if (0 != lotto_plugin_enable_only(plugin_list))
            exit(1);
        size_t added_preload_len = 0;
        lotto_plugin_foreach(_load_plugin, &added_preload_len);
        char added_preload_len_str[128];
        sys_sprintf(added_preload_len_str, "%lu%c", added_preload_len, '\0');
        setenv("LOTTO_ADDED_PRELOAD_LEN", added_preload_len_str, true);
        setenv("LOTTO_CLI_RESTARTED", "1", true);
        return execv(argv[0], argv);
    }


#if defined(LOTTO_EMBED_LIB) && LOTTO_EMBED_LIB == 0
    /* add arg0 path to LD_LIBRARY_PATH */
    {
        char resolved_path[PATH_MAX];
        ASSERT(realpath(argv[0], resolved_path) != 0 &&
               "Unable to resolve path. You must provide lotto's path or a "
               "real path alias.");
        preload_set_libpath(dirname(resolved_path));
    }
#endif

    const char *scmd_str = argv[subcmd_pos + 1];
    subcmd_t *scmd       = subcmd_find(scmd_str != NULL ? scmd_str : "-h");
    if (NULL == scmd) {
        if (NULL != scmd_str) {
            subcmd_t *scmd_closest = subcmd_find_closest(scmd_str);
            if (NULL != scmd_closest) {
                sys_fprintf(stderr, "error: Did you mean %s ?\n",
                            scmd_closest->name);
            }
        }
        sys_fprintf(stderr, "error: unknown command %s\n", scmd_str);
        return 1;
    }

    exec_info_t *exec_info  = get_exec_info();
    exec_info->hash_actual  = get_lotto_hash(arg0);
    exec_info->args = (scmd->group != SUBCMD_GROUP_OTHER) ?
                          ARGS(argc - subcmd_pos - 1, argv + subcmd_pos + 1) :
                          ARGS(argc - subcmd_pos, argv + subcmd_pos);

    exec_info->args.arg0 = arg0;
    flags_t *flags       = scmd->defaults();
    switch (
        flags_parse(flags, &exec_info->args, scmd->runtime_sel, scmd->sel)) {
        case FLAGS_PARSE_OK: {
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
        case FLAGS_PARSE_ERROR:
            return -1;
        default:
            ASSERT(0 && "should never reach here");
    }
}

static int
_load_plugin(plugin_t *plugin, void *arg)
{
    (void)arg;
    if (!(plugin->kind & PLUGIN_KIND_CLI))
        return 0;

    const char *ld_preload = getenv("LD_PRELOAD");
    size_t plugin_len = strlen(plugin->path);
    size_t new_ld_preload_len = plugin_len + 1;

    if (ld_preload) {
        new_ld_preload_len += strlen(ld_preload) + 1;
    }

    char *new_ld_preload = alloca(new_ld_preload_len);
    if (!new_ld_preload) {
        logger_errorf("error allocating memory for LD_PRELOAD\n");
        return 0;
    }

    size_t added_ld_preload_len = plugin_len;
    if (ld_preload) {
        snprintf(new_ld_preload, new_ld_preload_len, "%s:%s", plugin->path, ld_preload);
        added_ld_preload_len++;
    } else {
        snprintf(new_ld_preload, new_ld_preload_len, "%s", plugin->path);

    }

    if (setenv("LD_PRELOAD", new_ld_preload, 1) != 0) {
        logger_errorf("error setting LD_PRELOAD\n");
        return 0;
    }
    *(size_t*)arg += added_ld_preload_len;
    return 0;
}
