#ifndef LOTTO_QEMU_TRANSLATE_AARCH64_H
#define LOTTO_QEMU_TRANSLATE_AARCH64_H

#include <qemu-plugin.h>
#include <stdint.h>

void qemu_on_plugin_start(qemu_plugin_id_t id, const qemu_info_t *info,
                          int argc, char **argv);
void qemu_on_plugin_stop(void);
void qemu_dispatch_tb_translate(qemu_plugin_id_t id, struct qemu_plugin_tb *tb);
void qemu_on_tb_translate(qemu_plugin_id_t id, struct qemu_plugin_tb *tb);
uint64_t qemu_instruction_icount(void);

#endif
