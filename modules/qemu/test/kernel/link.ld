/*
 * Linker script for the qlotto bare-metal test kernel.
 *
 * QEMU virt machine loads -kernel images at 0x40000000 and jumps to the ELF
 * entry point.  RAM starts at the same address.
 *
 * Layout
 * ------
 *  0x40000000  .text.start   _start (must be first — QEMU jumps here)
 *  0x40000800  .text.vectors exception vector table (2 KiB aligned)
 *              .text         remaining code
 *              .rodata
 *              .data
 *              .bss          zero-initialised globals (mailbox, done[], slots…)
 *  [aligned]   stack area    NUM_CORES × STACK_SIZE, grows downward per core
 */

ENTRY(_start)

SECTIONS
{
    . = 0x40000000;

    .text : {
        KEEP(*(.text.start))        /* _start must be the very first bytes  */
        . = ALIGN(2048);
        KEEP(*(.text.vectors))      /* VBAR_EL1 target – 2 KiB aligned      */
        *(.text*)
        *(.gnu.linkonce.t*)
    }

    .rodata : { *(.rodata*) *(.gnu.linkonce.r*) }

    .data : { *(.data*) *(.gnu.linkonce.d*) }

    . = ALIGN(8);
    __bss_start = .;
    .bss : {
        *(.bss*)
        *(COMMON)
    }
    . = ALIGN(8);
    __bss_end = .;

    /* Per-core stacks: STACK_SIZE bytes each, growing downward.
     * Core k uses  [__stack_bottom + k*STACK_SIZE,
     *               __stack_bottom + (k+1)*STACK_SIZE).
     * boot.S sets  SP = __stack_bottom + (cpu_id + 1) * STACK_SIZE.        */
    . = ALIGN(8192);
    __stack_bottom = .;
    . += 4 * 8192;      /* NUM_CORES * STACK_SIZE */
    __stack_top = .;
}
