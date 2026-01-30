/* Callback functions before and after executing an intercepted function. */
.extern intercept_lookup_call
.extern intercept_after_call
.extern intercept_warn_call

/*
 * @param name name of intercepted function
 */
.macro add_interceptor name
        .section .rodata
.func_\name:
        .string "\name"

        .section .text
        .align 4
        .globl \name
        .type  \name, @function
\name:
       /* Push callee-save AND caller-save registers onto the stack. */
        prologue
        push_caller_save

        /* BEFORE_CALL: start intercept and find pointer to intercepted
         * function (return from intercept_lookup_call). */
        arg_func_name \name
        call_func intercept_lookup_call

        /* CALL: restore caller-save registers and perform real call. */
        pop_caller_save
        call_func_ptr

        /* AFTER_CALL: Save return value and finish book keeping. */
        push_retval
        arg_func_name \name
        call_func intercept_after_call

        /* Restore return value from CALL and callee-save registers. */
        pop_retval
        epilogue
        return

.end_\name:
	.size	\name, .- \name
.endm


.macro add_warn name
        .section .rodata
.func_\name:
        .string "\name"

        .section .text
        .align 4
        .globl \name
        .type \name, @function
\name:
        prologue
        push_caller_save

        arg_func_name \name
        call_func intercept_warn_call

        pop_caller_save
        call_func_ptr

        epilogue
        return
.end_\name:
        .size \name, .- \name
.endm
