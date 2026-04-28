#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/subcmd.h>

NEW_PUBLIC_CALLBACK_FLAG(QEMU_GDB, "G", "qemu-gdb", "",
                         "enable qemu gdb server", flag_off(), {
                             if (is_on(v)) {
                                 qemu_gdb_enable();
                             } else {
                                 qemu_gdb_disable();
                             }
                         })

FLAG_GETTER(qemu_gdb, QEMU_GDB)

NEW_PUBLIC_CALLBACK_FLAG(QEMU_GDB_WAIT, "W", "qemu-gdb-wait", "",
                         "stop exeution, wait for gdb", flag_off(),
                         { qemu_gdb_set_wait(is_on(v)); })

FLAG_GETTER(qemu_gdb_wait, QEMU_GDB_WAIT)

static void
_attach_qemu_gdb_flags(const char *name)
{
    subcmd_t *subcmd = subcmd_find(name);
    if (subcmd == NULL) {
        return;
    }
    flag_sel_add(subcmd->sel, flag_qemu_gdb());
    flag_sel_add(subcmd->sel, flag_qemu_gdb_wait());
}

ON_INITIALIZATION_PHASE({ _attach_qemu_gdb_flags("qemu"); })

LOTTO_MODULE_INIT()
