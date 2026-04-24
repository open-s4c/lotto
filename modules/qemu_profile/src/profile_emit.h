#ifndef LOTTO_MODULES_QEMU_PROFILE_EMIT_H
#define LOTTO_MODULES_QEMU_PROFILE_EMIT_H

#include <stdbool.h>
#include <stddef.h>

void qemu_profile_on_init(void);
void qemu_profile_on_fini(void);
void qemu_profile_on_translate(size_t insn_count);
void qemu_profile_on_memaccess(bool is_store);
void qemu_profile_on_udf_yield(void);

#endif
