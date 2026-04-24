/**
 * @file qemu.h
 * @brief Utilities to instrument QEMU guests.
 *
 * The functions in this header should only be used in the context of QEMU.
 * They will issue a fake opcode that is caught by Lotto and handled
 * accordingly. Calling these functions in programs running natively on the
 * host is undefined behavior.
 */
#ifndef LOTTO_QEMU_H
#define LOTTO_QEMU_H

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ucontext.h>

#include <lotto/bias.h>
#include <lotto/qemu/lotto_udf.h>


static inline bool
_lotto_qemu_is_returning_udf(uint32_t opcode)
{
    switch (opcode) {
        case LOTTO_TRAP_A64_VAL:
            return true;
        default:
            return false;
    }
}

static inline void
_lotto_qemu_handle_udf_fault(int sig, siginfo_t *si, void *arg)
{
    (void)sig;
    (void)si;
    ucontext_t *ctx = (ucontext_t *)arg;
    uint64_t pc     = (uint64_t)ctx->uc_mcontext.pc;
    uint32_t opcode = *(const uint32_t *)(uintptr_t)pc;

    if (_lotto_qemu_is_returning_udf(opcode)) {
        ctx->uc_mcontext.pc = pc + 4;
        return;
    }

    signal(sig, SIG_DFL);
}

static inline void
lotto_qemu_udf_returns_init(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_flags     = SA_SIGINFO;
    action.sa_sigaction = _lotto_qemu_handle_udf_fault;
    sigaction(SIGILL, &action, NULL);
    sigaction(SIGSEGV, &action, NULL);
}

/**
 * Aborts Lotto's execution and signals a failure.
 */
static inline void
lotto_fail(void)
{
    LOTTO_EXIT_FAIL;
}

/**
 * Halts Lotto's execution and signals success.
 */
static inline void
lotto_halt(void)
{
    LOTTO_EXIT_SUCCESS;
}

/**
 * Set the current Lotto bias policy from a QEMU guest.
 */
static inline void
lotto_qemu_bias_policy(bias_policy_t policy)
{
    LOTTO_BIAS_POLICY_A64((uint64_t)policy);
}

/**
 * Toggle the current Lotto bias policy from a QEMU guest.
 */
static inline void
lotto_qemu_bias_toggle(void)
{
    LOTTO_BIAS_TOGGLE_A64;
}

/**
 * Enable or disable QEMU-side instrumentation for the current guest CPU.
 *
 * While disabled, semantic QEMU callbacks such as memaccess and yield/WF
 * interception are suppressed until instrumentation is re-enabled.
 */
static inline void
lotto_qemu_instrument(bool enabled)
{
    LOTTO_QEMU_INSTRUMENT_A64((uint64_t)enabled);
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
