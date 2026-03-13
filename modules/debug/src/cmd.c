/*******************************************************************************
 * debug
 ******************************************************************************/
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "debug.h"
#include <lotto/cli/preload.h>
#include <lotto/cmake_variables.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/subcmd.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

#define DEBUG_GDB_SCRIPT "debug.gdb"
#define DEBUG_ADDR2LINE  "aarch64-linux-gnu-addr2line"
#define DEBUG_PLUGIN_PATHS                                                     \
    LOTTO_MODULE_BUILD_DIR                                                     \
    ":" LOTTO_MODULE_INSTALL_DIR ":" QLOTTO_MODULE_BUILD_DIR                   \
    ":" QLOTTO_MODULE_INSTALL_DIR ":" CMAKE_INSTALL_PREFIX                     \
    "/share/lotto"                                                             \
    ":" CMAKE_BINARY_DIR

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
    const char *cmd = flags_get_sval(flags, flag_before_run());
    if (cmd && cmd[0]) {
        sys_setenv("LOTTO_DISABLE", "true", true);
        int ret = sys_system(cmd);
        sys_unsetenv("LOTTO_DISABLE");
        ASSERT(ret != -1 && "could not run the before run action");
    }

    const char *gdb   = flags_get_sval(flags, FLAG_GDB_COMMAND);
    const char *input = flags_get_sval(flags, flag_input());
    char filename[PATH_MAX];
    const char *debug_dir = flags_get_sval(flags, flag_temporary_directory());
    ASSERT(debug_dump_assets(debug_dir) &&
           "could not dump debug assets, try specifying a different "
           "--temporary-directory CLI option");
    sys_sprintf(filename, "%s/%s", debug_dir, DEBUG_GDB_SCRIPT);
    sys_setenv("LOTTO_DEBUG_DIR", debug_dir, true);
    sys_setenv("LOTTO_DEBUG_FILE_FILTER",
               flags_get_sval(flags, FLAG_FILE_FILTER), true);
    sys_setenv("LOTTO_DEBUG_FUNCTION_FILTER",
               flags_get_sval(flags, FLAG_FUNCTION_FILTER), true);
    sys_setenv("LOTTO_DEBUG_ADDR2LINE", DEBUG_ADDR2LINE, true);
    sys_setenv("LOTTO_DEBUG_SYMBOL_FILE",
               flags_get_sval(flags, FLAG_SYMBOL_FILE), true);
    sys_setenv("LOTTO_DEBUG_PLUGIN_PATHS", DEBUG_PLUGIN_PATHS, true);
    const char *argv[] = {gdb,
                          "-x",
                          filename,
                          "--args",
                          args->arg0,
                          "replay",
                          "-i",
                          input,
                          "--temporary-directory",
                          flags_get_sval(flags, flag_temporary_directory()),
                          flags_is_on(flags, flag_verbose()) ? "-v" : NULL,
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

    res = execvpe(argv[0], (char *const *)argv, environ);
    if (res != 0) {
        sys_fprintf(stderr, "gdb unexpectedly quit with: %d\n", res);
    }

    cmd = flags_get_sval(flags, flag_after_run());
    if (cmd && cmd[0]) {
        sys_setenv("LOTTO_DISABLE", "true", true);
        int ret = sys_system(cmd);
        sys_unsetenv("LOTTO_DISABLE");
        ASSERT(ret != -1 && "could not run the after run action");
    }

    return res;
}

LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__INIT, {
    flag_t sel[] = {flag_input(),
                    flag_verbose(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_replay_goal(),
                    flag_before_run(),
                    flag_after_run(),
                    FLAG_SYMBOL_FILE,
                    FLAG_GDB_USE_MI,
                    FLAG_FUNCTION_FILTER,
                    FLAG_FILE_FILTER,
                    FLAG_GDB_COMMAND,
                    0};
    subcmd_register(debug, "debug", "", "Debug a trace", true, sel,
                    flags_default, SUBCMD_GROUP_TRACE);
})
