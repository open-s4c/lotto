/**
 * @file trap.h
 * @brief Runtime declarations for generic Lotto trap payloads.
 */
#ifndef LOTTO_RUNTIME_TRAP_H
#define LOTTO_RUNTIME_TRAP_H

#include <stdint.h>

typedef struct lotto_trap_event {
    uint64_t regs[5];
} lotto_trap_event_t;

#endif
