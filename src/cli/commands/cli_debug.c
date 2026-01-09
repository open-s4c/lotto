/*
 */
/*******************************************************************************
 * debug
 ******************************************************************************/
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/cli/flagmgr.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cmake_variables.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

DECLARE_FLAG_INPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_REPLAY_GOAL;
DECLARE_FLAG_BEFORE_RUN;
DECLARE_FLAG_AFTER_RUN;
DECLARE_COMMAND_FLAG(SYMBOL_FILE, "", "symbol-files", "FILE[:FILE]",
                     "files containing symbols", flag_sval(""))
DECLARE_COMMAND_FLAG(GDB_USE_MI, "", "use-mi", "", "use GDB/MI interface",
                     flag_off())
DECLARE_COMMAND_FLAG(FUNCTION_FILTER, "", "function-filter", "REGEX",
                     "library filter", flag_sval(""))
DECLARE_COMMAND_FLAG(FILE_FILTER, "", "file-filter", "REGEX", "file filter",
                     flag_sval(""))
DECLARE_COMMAND_FLAG(GDB_COMMAND, "", "gdb-command", "",
                     "specify the gdb command", flag_sval("gdb"))

int
debug(args_t *args, flags_t *flags)
{
    const char *cmd = flags_get_sval(flags, FLAG_BEFORE_RUN);
    if (cmd && cmd[0]) {
        sys_setenv("LOTTO_DISABLE", "true", true);
        int ret = sys_system(cmd);
        sys_unsetenv("LOTTO_DISABLE");
        ASSERT(ret != -1 && "could not run the before run action");
    }

    const char *gdb   = flags_get_sval(flags, FLAG_GDB_COMMAND);
    const char *input = flags_get_sval(flags, FLAG_INPUT);
    char filename[PATH_MAX];
    sys_sprintf(filename, "%s/%s",
                flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY), GDB);
    const char *argv[] = {gdb,
                          "-x",
                          filename,
                          "--args",
                          args->arg0,
                          "replay",
                          "-i",
                          input,
                          "--temporary-directory",
                          flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY),
                          flags_is_on(flags, FLAG_VERBOSE) ? "-v" : NULL,
                          NULL,
                          NULL};
    if (flags_is_on(flags, FLAG_GDB_USE_MI)) {
        sys_memmove(&argv[2], &argv[1], sizeof(argv) - 2 * sizeof(argv[0]));
        argv[1] = "-i=mi";
    }
    int res = sys_system("command -v gdb > /dev/null 2>&1");
    if (res != 0) {
        sys_fprintf(
            stderr,
            "Unable to find gdb. Please install gdb or add it to PATH.\n");
        return res;
    }

    debug_preload(flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY),
                  flags_get_sval(flags, FLAG_FILE_FILTER),
                  flags_get_sval(flags, FLAG_FUNCTION_FILTER),
                  "aarch64-linux-gnu-addr2line",
                  flags_get_sval(flags, FLAG_SYMBOL_FILE));

    res = execvpe(argv[0], (char *const *)argv, environ);
    if (res != 0) {
        sys_fprintf(stderr, "gdb unexpectedly quit with: %d\n", res);
    }

    cmd = flags_get_sval(flags, FLAG_AFTER_RUN);
    if (cmd && cmd[0]) {
        sys_setenv("LOTTO_DISABLE", "true", true);
        int ret = sys_system(cmd);
        sys_unsetenv("LOTTO_DISABLE");
        ASSERT(ret != -1 && "could not run the after run action");
    }

    return res;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_INPUT,
                    FLAG_VERBOSE,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_REPLAY_GOAL,
                    FLAG_BEFORE_RUN,
                    FLAG_AFTER_RUN,
                    FLAG_SYMBOL_FILE,
                    FLAG_GDB_USE_MI,
                    FLAG_FUNCTION_FILTER,
                    FLAG_FILE_FILTER,
                    FLAG_GDB_COMMAND,
                    0};
    subcmd_register(debug, "debug", "", "Debug a trace", true, sel,
                    flags_default, SUBCMD_GROUP_TRACE);
}
