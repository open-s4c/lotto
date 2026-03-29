#if !defined(__linux__)
#error "lotto_syscall is only implemented for Linux"
#elif defined(__x86_64__)

    .text
    .extern __errno_location

    .globl lotto_syscall
    .type lotto_syscall, @function
lotto_syscall:
    movq %rdi, %rax
    movq %rsi, %rdi
    movq %rdx, %rsi
    movq %rcx, %rdx
    movq %r8, %r10
    movq %r9, %r8
    movq 8(%rsp), %r9
    syscall

    cmpq $-4095, %rax
    jae .Llotto_syscall_errno
    ret

.Llotto_syscall_errno:
    negq %rax
    pushq %rax
    call __errno_location@PLT
    popq %rdx
    movl %edx, (%rax)
    movq $-1, %rax
    ret

    .size lotto_syscall, .-lotto_syscall

#elif defined(__aarch64__)

    .text
    .extern __errno_location

    .globl lotto_syscall
    .type lotto_syscall, %function
lotto_syscall:
    mov x8, x0
    mov x0, x1
    mov x1, x2
    mov x2, x3
    mov x3, x4
    mov x4, x5
    mov x5, x6
    svc #0

    mov x9, #-4095
    cmp x0, x9
    b.hs .Llotto_syscall_errno
    ret

.Llotto_syscall_errno:
    stp x19, x30, [sp, #-16]!
    neg x19, x0
    bl __errno_location
    str w19, [x0]
    mov x0, #-1
    ldp x19, x30, [sp], #16
    ret

    .size lotto_syscall, .-lotto_syscall

#else
#error "lotto_syscall is only implemented for x86_64 and aarch64"
#endif
