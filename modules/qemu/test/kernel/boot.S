/*
 * boot.S - AArch64 bare-metal entry for QEMU virt (-machine virt -smp 4)
 *
 * With -smp 4 QEMU starts all four cores simultaneously at the ELF entry
 * point.  Each core reads its ID from MPIDR_EL1, sets up its private stack,
 * then either runs primary_main() (core 0) or spins on its mailbox slot
 * waiting for core 0 to write a function pointer.
 *
 * Exception vector
 * ----------------
 * A minimal exception vector is installed so that if the guest takes an
 * unexpected exception (e.g. a UDF instruction when the qlotto plugin is NOT
 * loaded) it calls PSCI SYSTEM_RESET instead of looping silently at address 0.
 * When the qlotto plugin IS loaded it intercepts UDF instructions at TCG
 * translation time, so the guest never takes the exception.
 */

#define STACK_SIZE      8192
#define PSCI_SYSTEM_RESET 0x84000009

/* ============================================================
 * _start  –  placed first by the linker script (.text.start)
 * QEMU jumps here for every core.
 * ============================================================ */
    .section ".text.start", "ax"
    .global _start
    .type   _start, %function
_start:
    /* Mask IRQ, FIQ, Debug, SError at the current exception level. */
    msr     daifset, #0xf

    /* Use SP_EL1 so exceptions go to the EL1h (SPx) vector entries. */
    msr     spsel, #1
    isb

    /* Install exception vectors. */
    adr     x0, _vectors
    msr     vbar_el1, x0
    isb

    /* Determine CPU ID from MPIDR_EL1 bits [7:0] (Aff0). */
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff

    /*
     * Set per-core stack pointer.
     * Core k occupies [__stack_bottom + k*STACK_SIZE,
     *                   __stack_bottom + (k+1)*STACK_SIZE).
     * SP points at the top (exclusive upper bound) of the slot.
     */
    ldr     x1, =__stack_bottom
    mov     x2, #STACK_SIZE
    add     x3, x0, #1          /* k + 1                    */
    mul     x3, x3, x2          /* (k+1) * STACK_SIZE       */
    add     sp, x1, x3          /* __stack_bottom + offset  */

    /* Core 0 initialises BSS and enters C; others wait on the mailbox. */
    cbnz    x0, secondary_wait

    /* ---- Core 0 only ----------------------------------------- */
    /* Clear BSS (8 bytes at a time; linker guarantees 8-byte alignment). */
    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
.Lbss_loop:
    cmp     x1, x2
    b.ge    .Lbss_done
    str     xzr, [x1], #8
    b       .Lbss_loop
.Lbss_done:
    bl      primary_main
    /* primary_main should not return; trap here if it does. */
.Lprimary_halt:
    b       .Lprimary_halt

    .ltorg

/* ---- Secondary cores ----------------------------------------- */
secondary_wait:
    /*
     * Spin on mailbox[cpu_id] (8 bytes per slot).
     * When core 0 writes a function pointer, branch to it.
     * x0 still holds cpu_id — passed as first argument by the C calling
     * convention (AArch64 passes arg0 in x0).
     */
    ldr     x1, =mailbox
    lsl     x2, x0, #3          /* cpu_id * 8               */
    add     x1, x1, x2          /* &mailbox[cpu_id]         */
.Lspin:
    ldr     x3, [x1]
    cbz     x3, .Lspin
    br      x3                  /* jump to secondary_entry(cpu_id) */

    .ltorg


/* ============================================================
 * Exception vector table
 * Must be aligned to 2 KiB (2^11).  Each of the 16 entries is
 * exactly 128 bytes (2^7).  We route all synchronous exceptions
 * to _exception_reset; all other entries spin.
 * ============================================================ */
    .section ".text.vectors", "ax"
    .global _vectors
    .align  11
_vectors:
    /* --- Current EL, SP_EL0 (EL1t) --- */
    .align 7; b _exception_reset   /* 0x000 sync  */
    .align 7; b .                   /* 0x080 IRQ   */
    .align 7; b .                   /* 0x100 FIQ   */
    .align 7; b .                   /* 0x180 SError */

    /* --- Current EL, SP_ELx (EL1h) --- */
    .align 7; b _exception_reset   /* 0x200 sync  */
    .align 7; b .                   /* 0x280 IRQ   */
    .align 7; b .                   /* 0x300 FIQ   */
    .align 7; b .                   /* 0x380 SError */

    /* --- Lower EL, AArch64 --- */
    .align 7; b .                   /* 0x400 sync  */
    .align 7; b .                   /* 0x480 IRQ   */
    .align 7; b .                   /* 0x500 FIQ   */
    .align 7; b .                   /* 0x580 SError */

    /* --- Lower EL, AArch32 --- */
    .align 7; b .                   /* 0x600 sync  */
    .align 7; b .                   /* 0x680 IRQ   */
    .align 7; b .                   /* 0x700 FIQ   */
    .align 7; b .                   /* 0x780 SError */


/* ============================================================
 * _exception_reset
 * Called on any unexpected synchronous exception (including UDF
 * when no qlotto plugin is loaded).  Issues PSCI SYSTEM_RESET.
 * ============================================================ */
    .section ".text", "ax"
    .global _exception_reset
    .type   _exception_reset, %function
_exception_reset:
    movz    x0, #0x8400, lsl #16
    movk    x0, #0x0009             /* x0 = PSCI_SYSTEM_RESET */
    hvc     #0
    smc     #0
.Lreset_loop:
    b       .Lreset_loop
