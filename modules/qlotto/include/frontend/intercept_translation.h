/*
 */
#ifndef LOTTO_QEMU_INTERCEPT_TRANSLATION_H
#define LOTTO_QEMU_INTERCEPT_TRANSLATION_H

// qemu-plugin
#include <qemu-plugin.h>

// capstone
#include <capstone/arm64.h>
#include <capstone/capstone.h>

void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb);

void instrumentation_init(void);
void instrumentation_exit(void);

#endif // LOTTO_QEMU_INTERCEPT_TRANSLATION_H
