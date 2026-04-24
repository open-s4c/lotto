// REQUIRES: module-qemu
// RUN: rm -f %t.trace
// RUN: %lotto %trace -Q -- -kernel
// %builddir/modules/qemu/test/kernel/0001-kernel-1core-pass.elf RUN: %lotto
// %show | grep -E "args = .*qemu-system-aarch64 .* -kernel
// .*0001-kernel-1core-pass.elf" RUN: %lotto %stress -Q -r 1 -- -kernel
// %builddir/modules/qemu/test/kernel/0001-kernel-1core-pass.elf RUN: %lotto
// %show | grep -E "args = .*qemu-system-aarch64 .* -kernel
// .*0001-kernel-1core-pass.elf"

int
main(void)
{
    return 0;
}
