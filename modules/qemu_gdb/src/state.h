#ifndef LOTTO_STATE_QEMU_GDB_H
#define LOTTO_STATE_QEMU_GDB_H

#include <lotto/base/marshable.h>
#include <lotto/modules/qemu_gdb/qemu_gdb.h>

typedef struct qemu_gdb_config {
    marshable_t m;
    bool enabled;
    bool wait;
} qemu_gdb_config_t;

qemu_gdb_config_t *qemu_gdb_config(void);

#endif
