/*
 */
#ifndef LOTTO_QEMU_SYSCALL_AARCH64_H
#define LOTTO_QEMU_SYSCALL_AARCH64_H

#include <inttypes.h>

int lotto_qemu_do_translate(uint64_t pc) __attribute__((weak));
int lotto_qemu_do_yield(uint64_t pc) __attribute__((weak));
int lotto_qemu_get_syscall_nr(void) __attribute__((weak));

#endif
