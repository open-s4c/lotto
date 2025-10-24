/*
 */
#ifndef LOTTO_QEMU_H
#define LOTTO_QEMU_H
/**
 * @file qemu.h
 * @brief Utilities to instrument QEMU guests.
 *
 * The functions in this header should only be used in the context of QEMU.
 * They will issue a fake opcode that is caught by Lotto and handled
 * accordingly. Calling these functions in programs running natively on the
 * host is undefined behavior.
 */
#include <lotto/qemu/lotto_udf.h>

/**
 * Aborts Lotto's execution and signals a failure.
 */
static inline void
lotto_fail(void)
{
    LOTTO_TEST_FAIL;
}

/**
 * Halts Lotto's execution and signals success.
 */
static inline void
lotto_halt(void)
{
    LOTTO_TEST_SUCCESS;
}

/**
 * Sets QLotto plugin to inject fine-grained capture points.
 *
 * This function can be called recursively. An equal number of
 * `lotto_coarse_capture` has to be called to balance them out.
 */
void
lotto_fine_capture()
{
    LOTTO_REGION_IN;
}

/**
 * Sets QLotto plugin to inject grained-grained capture points.
 *
 * This function cancels the effect of `lotto_fine_capture` if called the same
 * number of consecutive times.
 */
void
lotto_coarse_capture()
{
    LOTTO_REGION_OUT;
}
#endif
