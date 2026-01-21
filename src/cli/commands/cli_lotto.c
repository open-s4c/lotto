/*
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/flag.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/memmgr.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/string.h>
#include <sys/types.h>

#ifndef LOTTO_VERSION
    #error "Lotto version undefined"
#endif

static const char *lotto_version = LOTTO_VERSION;

DECLARE_FLAG_VERSION;
DECLARE_FLAG_TEMPORARY_DIRECTORY;

DECLARE_COMMAND_FLAG(PLUGIN_DIRECTORY, "", "plugin-directory", "DIR",
                     "plugin directory to load Lotto plugins", flag_sval(""))
DECLARE_COMMAND_FLAG(PLUGIN_LIST, "", "plugins", "P1[,P2]",
                     "list of plugins to load, comma seperated", flag_sval(""))
DECLARE_COMMAND_FLAG(LIST_COMMANDS, "", "list-commands", "", "", flag_sval(""))

static void
print_block_fp(FILE *fp, const char *s, size_t len)
{
    sys_fprintf(fp, "%s", s);
    for (size_t i = 0; i + sys_strlen(s) < len; i++)
        sys_fprintf(fp, " ");
}

static void
show_version()
{
    sys_fprintf(stdout, "%s\n", lotto_version);
}

static void
list_commands(enum subcmd_group group)
{
    subcmd_t *s = NULL;
    while ((s = subcmd_next(s))) {
        if (s->group == group) {
            sys_fprintf(stdout, "%s\n", s->name);
        }
    }
}

static void
subcmds_group_help(FILE *fp, enum subcmd_group group, size_t max_name)
{
    subcmd_t *s = NULL;
    while ((s = subcmd_next(s))) {
        if (s->group == group) {
            sys_fprintf(fp, "    ");
            print_block_fp(fp, s->name, max_name + 2);
            sys_fprintf(fp, "%s\n", s->desc);
        }
    }
}

static void
subcmds_help(FILE *fp)
{
    subcmd_t *s     = NULL;
    size_t max_name = 0;
    size_t len;
    while ((s = subcmd_next(s))) {
        if (len = sys_strlen(s->name), len > max_name)
            max_name = len;
    }

    sys_fprintf(fp, "Commands for running programs and producing traces:\n");
    subcmds_group_help(fp, SUBCMD_GROUP_RUN, max_name);

    sys_fprintf(fp, "\nCommands for working further on traces:\n");
    subcmds_group_help(fp, SUBCMD_GROUP_TRACE, max_name);
}

static void
describe_usage(FILE *fp)
{
    sys_fprintf(fp, "Usage:\n");
    sys_fprintf(fp,
                "    lotto [--version|--help]|[--plugin-dir DIR]|[--plugins "
                "P1[,P2]] <command> <args>]\n\n");
    subcmds_help(fp);
}

/**
 * general lotto information
 */
static int
lotto(args_t *args, flags_t *flags)
{
    logger(LOGGER_INFO, stdout);

    if (flags_is_on(flags, FLAG_VERSION)) {
        show_version();
        return 0;
    }

    const char *cmds = flags_get_sval(flags, FLAG_LIST_COMMANDS);

    if (strcmp(cmds, "run") == 0) {
        list_commands(SUBCMD_GROUP_RUN);
        return 0;
    }
    if (strcmp(cmds, "trace") == 0) {
        list_commands(SUBCMD_GROUP_TRACE);
        return 0;
    }

    describe_usage(stdout);

    return 0;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_VERSION,       FLAG_TEMPORARY_DIRECTORY,
                    FLAG_LIST_COMMANDS, FLAG_PLUGIN_DIRECTORY,
                    FLAG_PLUGIN_LIST,   0};
    subcmd_register(lotto, "-", "", "Show details of lotto itself", false, sel,
                    flags_default, SUBCMD_GROUP_OTHER);
}
