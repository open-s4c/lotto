/**
 * @file final.h
 * @brief Final-state metadata for QEMU snapshots.
 */
#ifndef LOTTO_MODULES_QEMU_SNAPSHOT_FINAL_H
#define LOTTO_MODULES_QEMU_SNAPSHOT_FINAL_H

#include <stdbool.h>

#include <lotto/base/clk.h>
#include <lotto/base/marshable.h>

#ifndef QEMU_SNAPSHOT_NAME_MAX
    #define QEMU_SNAPSHOT_NAME_MAX 128
#endif

struct qemu_snapshot_final_state {
    marshable_t m;
    bool valid;
    bool success;
    clk_t clk;
    char name[QEMU_SNAPSHOT_NAME_MAX];
};

struct qemu_snapshot_final_state *qemu_snapshot_final_state(void);
void qemu_snapshot_final_note(const char *name, bool success);
void qemu_snapshot_final_set_clk(clk_t clk);

#endif
