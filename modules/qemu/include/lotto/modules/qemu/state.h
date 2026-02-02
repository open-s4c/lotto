/*
 */
#ifndef LOTTO_STATE_QEMU_H
#define LOTTO_STATE_QEMU_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct qemu_config {
    marshable_t m;
    bool gdb_enabled;
    bool gdb_wait;
} qemu_config_t;

qemu_config_t *qemu_config();

#endif
