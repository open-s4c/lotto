.set AUTOCEPT_CALL_NAME_OFFSET, 64
.set AUTOCEPT_CALL_FUNC_OFFSET, 72
.set AUTOCEPT_CALL_SRC_TYPE_OFFSET, 80
.set AUTOCEPT_CALL_RET_OFFSET, 88
.set AUTOCEPT_CALL_RETPC_OFFSET, 104
.set AUTOCEPT_CALL_FRAME_SIZE, 112
.set AUTOCEPT_VALUE_TYPE_UINT64, 2
.set AUTOCEPT_FP_CALL_ARGS_SIZE, 128
.set AUTOCEPT_FP_RETVAL_SIZE, 32

/* Build the BEFORE event on the current stack and pass it as the first C
 * argument to autocept_before.
 */
.macro prepare_before_event name, event
	/* Save the callee-save registers that the wrapper uses as scratch. */
	stp	x29, x30, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!

	/* Save q0..q7 because BEFORE publication may clobber FP argument regs. */
	sub	sp, sp, #AUTOCEPT_FP_CALL_ARGS_SIZE
	stp	q0, q1, [sp, #0]
	stp	q2, q3, [sp, #32]
	stp	q4, q5, [sp, #64]
	stp	q6, q7, [sp, #96]

	/* Build struct autocept_call_event directly on the stack. */
	sub	sp, sp, #AUTOCEPT_CALL_FRAME_SIZE
	stp	x0, x1, [sp, #0]
	stp	x2, x3, [sp, #16]
	stp	x4, x5, [sp, #32]
	stp	x6, x7, [sp, #48]
	adr	x9, .func_\name
	str	x9, [sp, #AUTOCEPT_CALL_NAME_OFFSET]
	adrp	x9, .autocept_fptr_\name
	ldr	x9, [x9, #:lo12:.autocept_fptr_\name]
	str	x9, [sp, #AUTOCEPT_CALL_FUNC_OFFSET]
	mov	w9, #\event
	strh	w9, [sp, #AUTOCEPT_CALL_SRC_TYPE_OFFSET]
	str	xzr, [sp, #AUTOCEPT_CALL_RET_OFFSET]
	str	xzr, [sp, #AUTOCEPT_CALL_RET_OFFSET + 8]
	str	x30, [sp, #AUTOCEPT_CALL_RETPC_OFFSET]

	/* Pass struct autocept_call_event * as the first C argument. */
	mov	x0, sp
.endm

/* Publish the BEFORE event and cache the resolved real function pointer. */
.macro publish_before_event name
	bl	autocept_before
	adrp	x9, .autocept_fptr_\name
	str	x0, [x9, #:lo12:.autocept_fptr_\name]
.endm

/* Tear down the temporary BEFORE frame and restore the original register/FP
 * argument view before the real call.
 */
.macro cleanup_before_event
	/* Restore the original GP argument registers from the event. */
	ldp	x0, x1, [sp, #0]
	ldp	x2, x3, [sp, #16]
	ldp	x4, x5, [sp, #32]
	ldp	x6, x7, [sp, #48]

	/* Drop the event frame and restore the saved FP call arguments. */
	add	sp, sp, #AUTOCEPT_CALL_FRAME_SIZE
	ldp	q0, q1, [sp, #0]
	ldp	q2, q3, [sp, #32]
	ldp	q4, q5, [sp, #64]
	ldp	q6, q7, [sp, #96]
	add	sp, sp, #AUTOCEPT_FP_CALL_ARGS_SIZE

	/* Drop the wrapper frame before the real call. */
	ldp	x19, x20, [sp], #16
	ldp	x29, x30, [sp], #16
.endm

/* Reload the cached real function pointer into x9 for the call. */
.macro prepare_fptr_call name
	adrp	x9, .autocept_fptr_\name
	ldr	x9, [x9, #:lo12:.autocept_fptr_\name]
.endm

/* Call the real wrapped function through the cached pointer. */
.macro call_fptr
	blr	x9
.endm

/* Preserve the real return value immediately after the wrapped call returns so
 * autocept_after can run without disturbing the caller-visible result.
 */
.macro cleanup_fptr_call
	/* Rebuild the wrapper's scratch frame for the AFTER path. */
	stp	x29, x30, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!

	/* Save the integer and FP return registers across autocept_after. */
	stp	x0, x1, [sp, #-16]!
	sub	sp, sp, #AUTOCEPT_FP_RETVAL_SIZE
	stp	q0, q1, [sp, #0]
.endm

/* Build the AFTER event from the real return path, write the captured scalar
 * return into ev->ret, and pass the rebuilt event to autocept_after.
 */
.macro prepare_after_event name, event
	/* Build a fresh event frame from the post-call register state. */
	sub	sp, sp, #AUTOCEPT_CALL_FRAME_SIZE
	stp	x0, x1, [sp, #0]
	stp	x2, x3, [sp, #16]
	stp	x4, x5, [sp, #32]
	stp	x6, x7, [sp, #48]
	adr	x9, .func_\name
	str	x9, [sp, #AUTOCEPT_CALL_NAME_OFFSET]
	adrp	x9, .autocept_fptr_\name
	ldr	x9, [x9, #:lo12:.autocept_fptr_\name]
	str	x9, [sp, #AUTOCEPT_CALL_FUNC_OFFSET]
	mov	w9, #\event
	strh	w9, [sp, #AUTOCEPT_CALL_SRC_TYPE_OFFSET]
	str	xzr, [sp, #AUTOCEPT_CALL_RET_OFFSET]
	str	xzr, [sp, #AUTOCEPT_CALL_RET_OFFSET + 8]
	str	x30, [sp, #AUTOCEPT_CALL_RETPC_OFFSET]

	/* Write the captured scalar return into ev->ret. */
	mov	w9, #AUTOCEPT_VALUE_TYPE_UINT64
	str	w9, [sp, #AUTOCEPT_CALL_RET_OFFSET]
	ldr	x9, [sp, #AUTOCEPT_CALL_FRAME_SIZE + AUTOCEPT_FP_RETVAL_SIZE]
	str	x9, [sp, #AUTOCEPT_CALL_RET_OFFSET + 8]

	/* Pass struct autocept_call_event * as the first C argument. */
	mov	x0, sp
.endm

/* Publish the AFTER event. */
.macro publish_after_event
	bl	autocept_after
.endm

/* Restore the saved original return address from the rebuilt event, then
 * recover the caller-visible return registers and wrapper state.
 */
.macro cleanup_after_event
	/* Recover the original return address from the rebuilt event. */
	ldr	x9, [sp, #AUTOCEPT_CALL_RETPC_OFFSET]

	/* Drop the event frame and restore the FP return registers. */
	add	sp, sp, #AUTOCEPT_CALL_FRAME_SIZE
	ldp	q0, q1, [sp, #0]
	add	sp, sp, #AUTOCEPT_FP_RETVAL_SIZE

	/* Restore the integer return registers and the wrapper frame. */
	ldp	x0, x1, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x29, x30, [sp], #16
	mov	x30, x9
.endm

.macro return
	ret
.endm
