/**
 * @file guest.h
 * @brief Minimal QEMU guest trap helpers.
 *
 * This header is safe to include from both userspace and guest kernel code.
 * It intentionally avoids userspace signal or libc dependencies.
 */
#ifndef LOTTO_QEMU_GUEST_H
#define LOTTO_QEMU_GUEST_H

#include <lotto/modules/terminate/events.h>
#include <lotto/qemu/lotto_udf_aarch64.h>

#define LOTTO_GUEST_TRAP_A64(op)                                               \
    AARCH64_TRAP0(UDF_AARCH64_INSTR_LOTTO_TRAP_VAL, op)

#define LOTTO_GUEST_EXIT_SUCCESS LOTTO_GUEST_TRAP_A64(EVENT_TERMINATE_SUCCESS)
#define LOTTO_GUEST_EXIT_FAILURE LOTTO_GUEST_TRAP_A64(EVENT_TERMINATE_FAILURE)

/**
 * Aborts Lotto's execution and signals a failure.
 */
static inline void
lotto_fail(void)
{
    LOTTO_GUEST_EXIT_FAILURE;
}

/**
 * Halts Lotto's execution and signals success.
 */
static inline void
lotto_halt(void)
{
    LOTTO_GUEST_EXIT_SUCCESS;
}

/**
 * Sets QLotto plugin to inject fine-grained capture points.
 *
 * This function can be called recursively. An equal number of
 * `lotto_coarse_capture` has to be called to balance them out.
 */
static inline void
lotto_fine_capture(void)
{
    // LOTTO_REGION_IN;
}

/**
 * Sets QLotto plugin to inject grained-grained capture points.
 *
 * This function cancels the effect of `lotto_fine_capture` if called the same
 * number of consecutive times.
 */
static inline void
lotto_coarse_capture(void)
{
    // LOTTO_REGION_OUT;
}

#endif
