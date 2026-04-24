// clang-format off
// REQUIRES: module-qemu
// REQUIRES: module-qemu_profile
// RUN: LOTTO_QEMU_PROFILE=1 %lotto %stress -Q -r 1 -- -smp 1 -kernel %builddir/modules/qemu/test/kernel/0008-kernel-1core-profile.elf 2>&1 | grep "qemu-profile: tb="
// clang-format on

#include "kernel/kernel.h"
#include "kernel/uart.h"

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id != 0) {
        return;
    }

    uart_puts("profile: PASS\r\n");
    LOTTO_EXIT_SUCCESS;
}
