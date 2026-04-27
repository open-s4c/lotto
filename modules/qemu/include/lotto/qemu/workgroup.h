/**
 * @file workgroup.h
 * @brief QEMU workgroup helpers.
 */
#ifndef LOTTO_QEMU_WORKGROUP_H
#define LOTTO_QEMU_WORKGROUP_H

#include <lotto/qemu/trap.h>

static inline void
qlotto_workgroup_join(void)
{
    LOTTO_WORKGROUP_JOIN_A64;
}

#endif
