/*
 */
/*******************************************************************************
 * sc
 ******************************************************************************/
#include <dirent.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/envvar.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/cli/args.h>
#include <lotto/cli/exec.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/handlers/capture_group.h>
#include <lotto/cli/flags/memmgr.h>
#include <lotto/cli/flags/prng.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/cli/utils.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <sys/stat.h>

void round_print(flags_t *flags, uint64_t round);

#define SC_DIR "sc"

DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_ROUNDS;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_LOGGER_BLOCK;
DECLARE_FLAG_CREP;
DECLARE_COMMAND_FLAG(SC_EXPLORATION_ROUNDS, "", "sc-exploration-rounds", "INT",
                     "SC exploration rounds", flag_uval(MAX_ROUNDS))

/**
 * stress test
 */
int
sc(args_t *args, flags_t *flags)
{
    preload(flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY),
            flags_is_on(flags, FLAG_VERBOSE),
            !flags_is_on(flags, FLAG_NO_PRELOAD), flags_is_on(flags, FLAG_CREP),
            false, flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    envvar_t vars[] = {
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, FLAG_OUTPUT)},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        {NULL}};
    envvar_set(vars, true);
    flags_set_by_opt(flags, flag_delayed_atomic(), on());

    if (flags_is_on(flags, FLAG_VERBOSE)) {
        sys_fprintf(stdout, "[lotto] starting: ");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }

    char dirname[PATH_MAX];
    sys_sprintf(dirname, "%s/%s",
                flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY), SC_DIR);
    struct stat stats = {0};
    if (stat(dirname, &stats) == -1) {
        // create new directory if necessary
        mkdir(dirname, 0700);
    } else {
        // clean up
        DIR *dp = opendir(dirname);
        ASSERT(dp);
        for (struct dirent *file; (file = readdir(dp));) {
            if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, ".."))
                continue;
            char filename[PATH_MAX];
            sys_sprintf(filename, "%s/%s", dirname, file->d_name);
            remove(filename);
        }
    }

    sys_fprintf(stdout, "[lotto] exploring SC states\n");
    int err = 0;
    for (unsigned i = 0; i < flags_get_uval(flags, FLAG_SC_EXPLORATION_ROUNDS);
         i++) {
        round_print(flags, i);
        char filename[PATH_MAX];
        sys_sprintf(filename, "%s/%d%s", dirname, i, ".tidmap");
        struct value val = sval(filename);
        PS_PUBLISH_INTERFACE(TOPIC_DELAYED_PATH, val);
        envvar_set(vars, true);
        if ((err = execute(args, flags, false)))
            return err;

        adjust(flags_get_sval(flags, FLAG_OUTPUT));
    }

    sys_fprintf(stdout, "[lotto] exploring non-SC states\n");
    flags_set_by_opt(flags, flag_delayed_atomic(), off());
    struct value val = sval(dirname);
    PS_PUBLISH_INTERFACE(TOPIC_DELAYED_PATH, val);
    for (unsigned i = 0; i < flags_get_uval(flags, FLAG_ROUNDS); i++) {
        round_print(flags, i);

        struct value val = uval(now());
        flags_set_by_opt(flags, flag_seed(), val);

        if ((err = execute(args, flags, false)))
            return err;

        adjust(flags_get_sval(flags, FLAG_OUTPUT));
    }
    return 0;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_OUTPUT,
                    FLAG_VERBOSE,
                    FLAG_ROUNDS,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_SC_EXPLORATION_ROUNDS,
                    FLAG_CREP,
                    0};
    subcmd_register(
        sc, "sc", "[--] <command line>",
        "Check sequential consistency of a program with capture group", true,
        sel, flags_default, SUBCMD_GROUP_RUN);
}
