/**
 * @file snapshot.h
 * @brief QEMU snapshot helpers.
 */
#ifndef LOTTO_QEMU_SNAPSHOT_H
#define LOTTO_QEMU_SNAPSHOT_H

#include <lotto/qemu/trap.h>

static inline void
qlotto_snapshot(void)
{
    LOTTO_SNAPSHOT_A64;
}

#endif
