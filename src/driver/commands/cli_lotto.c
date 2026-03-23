#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/flag.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/memmgr.h>
#include <lotto/driver/preload.h>
#include <lotto/driver/subcmd.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/string.h>
#include <sys/types.h>

#ifndef LOTTO_VERSION
    #error "Lotto version undefined"
#endif

static const char *lotto_version = LOTTO_VERSION;

DECLARE_FLAG_VERSION;

DECLARE_COMMAND_FLAG(PLUGIN_DIRECTORY, "", "plugin-directory", "DIR",
                     "plugin directory to load Lotto plugins", flag_sval(""))
DECLARE_COMMAND_FLAG(PLUGIN_LIST, "", "plugins", "P1[,P2]",
                     "list of plugins to load, comma seperated", flag_sval(""))
DECLARE_COMMAND_FLAG(LIST_COMMANDS, "", "list-commands", "",
                     "list available commands", flag_off())

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
list_commands_all(void)
{
    subcmd_t *s = NULL;
    while ((s = subcmd_next(s))) {
        if (s->name[0] == '-') {
            continue;
        }
        sys_fprintf(stdout, "%s\t%s\n", s->name, s->desc);
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
    logger(LOGGER_INFO, STDOUT_FILENO);

    if (flags_is_on(flags, FLAG_VERSION)) {
        show_version();
        return 0;
    }

    if (flags_is_on(flags, FLAG_LIST_COMMANDS)) {
        list_commands_all();
        return 0;
    }

    describe_usage(stdout);

    return 0;
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {FLAG_VERSION,       flag_temporary_directory(),
                    FLAG_LIST_COMMANDS, FLAG_PLUGIN_DIRECTORY,
                    FLAG_PLUGIN_LIST,   0};
    subcmd_register(lotto, "-", "", "Show details of lotto itself", false, sel,
                    flags_default, SUBCMD_GROUP_OTHER);
})
