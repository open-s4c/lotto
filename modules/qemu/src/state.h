/**
 * @file state.h
 * @brief QEMU module declarations for state.
 */
#ifndef LOTTO_STATE_QEMU_H
#define LOTTO_STATE_QEMU_H

#include <lotto/base/flag.h>
#include <lotto/base/flags.h>
#include <lotto/driver/args.h>

void qemu_enable_stress_prefix(bool enabled);
void qemu_prefix_args(args_t *args, const flags_t *flags);
void qemu_resolve_replay_args(args_t *args, const flags_t *flags);
flag_t flag_stress_qemu();
flag_t flag_qemu_bin();
flag_t flag_qemu_plugin_debug();
flag_t flag_qemu_plugins();
flag_t flag_qemu_mem();
flag_t flag_qemu_cpu();
flag_t flag_qemu_no_common_args();

#endif
