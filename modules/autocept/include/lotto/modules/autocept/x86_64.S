/*
 * x86_64 SysV ABI notes used by the macros below:
 * - GP args 0..5 live in rdi, rsi, rdx, rcx, r8, r9
 * - FP args 0..7 live in xmm0..xmm7
 * - integer/pointer returns use rax, optionally rdx
 * - floating-point returns use xmm0, optionally xmm1
 *
 * Autocept publishes before/after events around a real call. That means the
 * interceptor must temporarily preserve the incoming call ABI while it runs
 * helper C code, then restore the original ABI exactly before the real call.
 */

.set AUTOCEPT_CALL_NAME_OFFSET, 64
.set AUTOCEPT_CALL_FUNC_OFFSET, 72
.set AUTOCEPT_CALL_SRC_TYPE_OFFSET, 80
.set AUTOCEPT_CALL_RET_OFFSET, 88
.set AUTOCEPT_CALL_RETPC_OFFSET, 104
.set AUTOCEPT_CALL_SAVED_CALLEE_OFFSET, 112
.set AUTOCEPT_CALL_EVENT_SIZE, 120
.set AUTOCEPT_VALUE_TYPE_UINT64, 2
.set AUTOCEPT_MD_SIZE, 8
.set AUTOCEPT_MD_OFFSET, AUTOCEPT_CALL_EVENT_SIZE
.set AUTOCEPT_CALL_FRAME_SIZE, AUTOCEPT_CALL_EVENT_SIZE + AUTOCEPT_MD_SIZE
.set AUTOCEPT_FP_CALL_ARGS_SIZE, 128
.set AUTOCEPT_FP_RETVAL_SIZE, 32

/* Build the BEFORE event on the current stack and pass it as the first C
 * argument to autocept_before.
 */
.macro prepare_before_event name, event
        /* Save the callee-save registers that autocept itself uses as scratch
         * across helper calls. r14 also keeps the original incoming rax/al so
         * variadic calls can restore the SysV vector-argument count later.
         */
        pushq  %rbx
        pushq  %r12
        pushq  %r13
        pushq  %r14
        pushq  %r15
        movq   %rax, %r14

        /* Save xmm0..xmm7 because BEFORE publication may call arbitrary C code
         * that clobbers FP argument registers.
         */
        subq  $AUTOCEPT_FP_CALL_ARGS_SIZE, %rsp
        movdqu %xmm0, 0(%rsp)
        movdqu %xmm1, 16(%rsp)
        movdqu %xmm2, 32(%rsp)
        movdqu %xmm3, 48(%rsp)
        movdqu %xmm4, 64(%rsp)
        movdqu %xmm5, 80(%rsp)
        movdqu %xmm6, 96(%rsp)
        movdqu %xmm7, 112(%rsp)

        /* Build struct autocept_call_event directly on the stack from the
         * current register view plus the symbol metadata.
         */
        subq  $AUTOCEPT_CALL_FRAME_SIZE, %rsp
        movq  %rdi, 0(%rsp)
        movq  %rsi, 8(%rsp)
        movq  %rdx, 16(%rsp)
        movq  %rcx, 24(%rsp)
        movq  %r8, 32(%rsp)
        movq  %r9, 40(%rsp)
        movq  %r10, 48(%rsp)
        movq  %r11, 56(%rsp)
        leaq  .func_\name(%rip), %rax
        movq  %rax, AUTOCEPT_CALL_NAME_OFFSET(%rsp)
        movq  .autocept_fptr_\name(%rip), %rax
        movq  %rax, AUTOCEPT_CALL_FUNC_OFFSET(%rsp)
        movw  $\event, AUTOCEPT_CALL_SRC_TYPE_OFFSET(%rsp)
        movq  $0, AUTOCEPT_CALL_RET_OFFSET(%rsp)
        movq  $0, AUTOCEPT_CALL_RET_OFFSET + 8(%rsp)
        movq  AUTOCEPT_CALL_FRAME_SIZE + AUTOCEPT_FP_CALL_ARGS_SIZE + 40(%rsp), %rax
        movq  %rax, AUTOCEPT_CALL_RETPC_OFFSET(%rsp)
        movq  %r15, AUTOCEPT_CALL_SAVED_CALLEE_OFFSET(%rsp)
        movq  $0, AUTOCEPT_MD_OFFSET(%rsp)

        /* Pass struct autocept_call_event * and metadata_t * to the helper. */
        movq  %rsp, %rdi
        leaq  AUTOCEPT_MD_OFFSET(%rsp), %rsi
.endm

/* Publish the BEFORE event. The helper returns the resolved real function
 * pointer in rax, which is cached per symbol. r10 preserves the original
 * incoming rax/al across epilogue so variadic calls can restore %al later.
 */
.macro publish_before_event name
        call  *autocept_before@GOTPCREL(%rip)
        movq  %rax, .autocept_fptr_\name(%rip)
        movq  %r14, %r10
        movzbq AUTOCEPT_MD_OFFSET(%rsp), %r15
.endm

/* Tear down the temporary BEFORE frame and restore the original register/FP
 * argument view. The wrapper's own frame is removed here so stacked user
 * arguments return to their original positions.
 */
.macro cleanup_before_event
        /* Restore the real SysV GP argument registers from the event. r10/r11
         * stay available as wrapper scratch.
         */
        movq  0(%rsp), %rdi
        movq  8(%rsp), %rsi
        movq  16(%rsp), %rdx
        movq  24(%rsp), %rcx
        movq  32(%rsp), %r8
        movq  40(%rsp), %r9

        /* Drop the temporary event and restore the saved FP call arguments. */
        addq  $AUTOCEPT_CALL_FRAME_SIZE, %rsp
        movdqu 0(%rsp), %xmm0
        movdqu 16(%rsp), %xmm1
        movdqu 32(%rsp), %xmm2
        movdqu 48(%rsp), %xmm3
        movdqu 64(%rsp), %xmm4
        movdqu 80(%rsp), %xmm5
        movdqu 96(%rsp), %xmm6
        movdqu 112(%rsp), %xmm7
        addq   $AUTOCEPT_FP_CALL_ARGS_SIZE, %rsp

        /* Remove the wrapper's own frame so stacked user arguments return to
         * their original positions before the real call. Keep r15 as the
         * metadata transport register; the original caller r15 is restored
         * later from the saved BEFORE event.
         */
        addq  $8, %rsp
        popq  %r14
        popq  %r13
        popq  %r12
        popq  %rbx
.endm

/* Recreate the final pieces of the original call ABI right before invoking the
 * real function. save_retpc removes the wrapped caller return address so the
 * continuation pushed by call_fptr lands directly above the original stacked
 * arguments. %al carries the SysV variadic vector-argument count.
 */
.macro prepare_fptr_call name
        addq  $8, %rsp
        movb  %r10b, %al
        movq  .autocept_fptr_\name(%rip), %r11
.endm

/* Call the real wrapped function through the cached pointer. */
.macro call_fptr
        call  *%r11
.endm

/* Preserve the real return value immediately after the wrapped call returns so
 * the AFTER publication can freely clobber caller-save and FP return registers.
 */
.macro cleanup_fptr_call
        /* Rebuild the wrapper's scratch frame for the AFTER path. */
        pushq  %rbx
        pushq  %r12
        pushq  %r13
        pushq  %r14
        pushq  %r15

        /* Move the integer return value out of caller-save registers before
         * autocept_after runs.
         */
        movq  %rax, %r12
        movq  %rdx, %r13

        /* Save xmm0/xmm1 because AFTER publication may clobber FP returns. */
        subq  $AUTOCEPT_FP_RETVAL_SIZE, %rsp
        movdqu %xmm0, 0(%rsp)
        movdqu %xmm1, 16(%rsp)
.endm

/* Build the AFTER event from the real return path, write the captured scalar
 * return into ev->ret, and pass the rebuilt event to autocept_after. The extra
 * 8-byte pad keeps the helper call 16-byte aligned on x86_64.
 */
.macro prepare_after_event name, event
        /* Build a fresh event frame from the post-call register state. */
        subq  $AUTOCEPT_CALL_FRAME_SIZE, %rsp
        movq  %rdi, 0(%rsp)
        movq  %rsi, 8(%rsp)
        movq  %rdx, 16(%rsp)
        movq  %rcx, 24(%rsp)
        movq  %r8, 32(%rsp)
        movq  %r9, 40(%rsp)
        movq  %r10, 48(%rsp)
        movq  %r11, 56(%rsp)
        leaq  .func_\name(%rip), %rax
        movq  %rax, AUTOCEPT_CALL_NAME_OFFSET(%rsp)
        movq  .autocept_fptr_\name(%rip), %rax
        movq  %rax, AUTOCEPT_CALL_FUNC_OFFSET(%rsp)
        movw  $\event, AUTOCEPT_CALL_SRC_TYPE_OFFSET(%rsp)
        movq  $0, AUTOCEPT_CALL_RET_OFFSET(%rsp)
        movq  $0, AUTOCEPT_CALL_RET_OFFSET + 8(%rsp)
        movq  AUTOCEPT_CALL_FRAME_SIZE + AUTOCEPT_FP_CALL_ARGS_SIZE + 40(%rsp), %rax
        movq  %rax, AUTOCEPT_CALL_RETPC_OFFSET(%rsp)
        movq  $0, AUTOCEPT_CALL_SAVED_CALLEE_OFFSET(%rsp)

        /* Write the captured scalar return into ev->ret. */
        movl  $AUTOCEPT_VALUE_TYPE_UINT64, AUTOCEPT_CALL_RET_OFFSET(%rsp)
        movq  %r12, AUTOCEPT_CALL_RET_OFFSET + 8(%rsp)
        movq  %r15, %rax
        movq  %rax, AUTOCEPT_MD_OFFSET(%rsp)

        /* Pass struct autocept_call_event * and metadata_t * to the helper,
         * then align the helper call to a 16-byte boundary.
         */
        movq  %rsp, %rdi
        leaq  AUTOCEPT_MD_OFFSET(%rsp), %rsi
        subq  $8, %rsp
.endm

/* Publish the AFTER event. */
.macro publish_after_event
        call  *autocept_after@GOTPCREL(%rip)
.endm

/* Drop the helper-call alignment pad, recover the saved original return
 * address from the rebuilt event, then restore the caller-visible return
 * registers and wrapper state. Pushing the saved retpc reconstructs the
 * original caller stack right before the final ret.
 */
.macro cleanup_after_event
        /* Drop the helper-call alignment pad and recover the original caller
         * return address from the rebuilt event.
         */
        addq  $8, %rsp
        movq  AUTOCEPT_CALL_RETPC_OFFSET(%rsp), %r11
        movq  AUTOCEPT_CALL_SAVED_CALLEE_OFFSET(%rsp), %r10

        /* Drop the event frame and restore the FP return registers. */
        addq  $AUTOCEPT_CALL_FRAME_SIZE, %rsp
        movdqu 0(%rsp), %xmm0
        movdqu 16(%rsp), %xmm1
        addq   $AUTOCEPT_FP_RETVAL_SIZE, %rsp

        /* Restore the integer return value right before returning. */
        movq  %r12, %rax
        movq  %r13, %rdx

        /* Drop the wrapper frame and reconstruct the original caller stack. */
        addq  $8, %rsp
        popq  %r14
        popq  %r13
        popq  %r12
        popq  %rbx
        movq  %r10, %r15
        pushq %r11
.endm

/* Return to the original caller. */
.macro return
        ret
.endm
