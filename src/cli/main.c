/*
 */
#include <dlfcn.h>
#include <errno.h>
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
        const int new_personality = personality(ADDR_NO_RANDOMIZE);
        if (new_personality & ADDR_NO_RANDOMIZE) {
            execv(argv[0], argv);
        }
    }

    const char *plugin_dir  = NULL;
    const char *plugin_list = NULL;
    int subcmd_pos          = 0;
    char *arg0              = argv[0];
    exec_info_t *exec_info  = get_exec_info();
    exec_info->hash_actual  = get_lotto_hash(arg0);

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

    lotto_plugin_scan(LOTTO_PLUGIN_BUILD_DIR, LOTTO_PLUGIN_INSTALL_DIR,
                      plugin_dir);
    if (0 != lotto_plugin_enable_only(plugin_list))
        exit(1);
    lotto_plugin_foreach(_load_plugin, NULL);

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
    void *handle = dlopen(plugin->path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        logger_errorf("error loading plugin '%s': %s\n", plugin->path, dlerror());
        plugin->disabled = true;
    }
    return 0;
}
