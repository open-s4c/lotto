/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <state.h>
#include <stdlib.h>

#include <lotto/cli/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_QEMU_GDB_ENABLED, "G", "gdb",
                         "enable qemu gdb server", flag_off(),
                         STR_CONVERTER_BOOL, {
                             setenv("LOTTO_QEMU_GDB", is_on(v) ? "1" : "0",
                                    true);
                         })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_QEMU_GDB_WAIT, "S", "gdb-wait",
                         "stop exeution, wait for gdb", flag_off(),
                         STR_CONVERTER_BOOL, {
                             setenv("LOTTO_QEMU_GDB_WAIT", is_on(v) ? "1" : "0",
                                    true);
                         })
