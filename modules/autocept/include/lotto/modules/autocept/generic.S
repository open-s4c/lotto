/* Callback functions before and after executing an intercepted function. */
.extern autocept_before
.extern autocept_after

/**
 * Macro to interpose function symbol without defining a precise signature.
 *
 * The macro sends events before/after calling the real function.
 *
 * @param name name of intercepted function, epoll_wait
 */
.macro add_interceptor name, event=0
        .section .rodata
.func_\name:
        .string "\name"

        .section .bss
        .align 8
.autocept_fptr_\name:
        .zero 8

        .section .text
        .align 4
        .globl \name
        .type  \name, @function
\name:
        /* BEFORE: build the stack event and publish it. CAPTURE_BEFORE copies
         * the event, including ev->retpc, into self TLS before the wrapper
         * restores the real call ABI.
         */
        prepare_before_event \name, \event
        publish_before_event \name
        cleanup_before_event

        /* CALL: tear down the temporary BEFORE frame, restore the original
         * function-call ABI, and invoke the resolved target.
         */
        prepare_fptr_call \name
        call_fptr

.continue_\name:
        /* AFTER: rebuild a fresh event from the real return path, publish it,
         * then restore the caller-visible return registers and return address.
         */
        cleanup_fptr_call

        prepare_after_event \name, \event
        publish_after_event
        cleanup_after_event

        return

.end_\name:
	.size	\name, .- \name
.endm
