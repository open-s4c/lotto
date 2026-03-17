/**
 * @file qemu_plugin.h
 * @brief QEMU module declarations for qemu plugin.
 */
#ifndef LOTTO_MODULES_QEMU_PLUGIN_H
#define LOTTO_MODULES_QEMU_PLUGIN_H

#include <lotto/base/task_id.h>
#include <lotto/modules/qemu/armcpu.h>
#include <lotto/qlotto/gdb_base.h>
#include <lotto/sys/assert.h>

void qemu_snapshot(void) __attribute__((weak));

__attribute__((weak)) void
qemu_snapshot(void)
{
    ASSERT(0 && "Should not call weak function, but qemu-plugin variant.");
}

#endif
