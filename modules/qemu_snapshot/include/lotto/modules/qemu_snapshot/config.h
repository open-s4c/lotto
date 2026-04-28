/**
 * @file config.h
 * @brief Config-state controls for QEMU snapshots.
 */
#ifndef LOTTO_MODULES_QEMU_SNAPSHOT_CONFIG_H
#define LOTTO_MODULES_QEMU_SNAPSHOT_CONFIG_H

#include <stdbool.h>

#include <lotto/base/clk.h>
#include <lotto/base/marshable.h>

#ifndef QEMU_SNAPSHOT_NAME_MAX
    #define QEMU_SNAPSHOT_NAME_MAX 128
#endif

struct qemu_snapshot_config_state {
    marshable_t m;
    bool enabled;
    clk_t clk;
    bool snapshot_valid;
    bool snapshot_success;
    clk_t snapshot_clk;
    char snapshot_name[QEMU_SNAPSHOT_NAME_MAX];
};

struct qemu_snapshot_config_state *qemu_snapshot_config_state(void);

#endif
