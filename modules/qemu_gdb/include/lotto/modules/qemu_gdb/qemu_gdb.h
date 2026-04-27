/**
 * @file qemu_gdb.h
 * @brief Driver/runtime declarations for the qemu_gdb module.
 */
#ifndef LOTTO_MODULES_QEMU_GDB_H
#define LOTTO_MODULES_QEMU_GDB_H

#include <stdbool.h>

#include <lotto/base/flag.h>

bool qemu_gdb_enabled(void);
bool qemu_gdb_get_wait(void);
void qemu_gdb_set_wait(bool wait);
void qemu_gdb_enable(void);
void qemu_gdb_disable(void);

flag_t flag_qemu_gdb(void);
flag_t flag_qemu_gdb_wait(void);

#endif
