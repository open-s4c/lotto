/**
 * @file trap.h
 * @brief QEMU guest trap declarations and payloads.
 */
#ifndef LOTTO_QEMU_TRAP_H
#define LOTTO_QEMU_TRAP_H

#include <stdint.h>

#include <lotto/qemu/lotto_udf.h>

struct lotto_trap_event {
    uint64_t regs[5];
};

#endif
