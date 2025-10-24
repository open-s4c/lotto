/*
 */

.section .note.GNU-stack,"",@progbits

#if defined(__x86_64__)
#include "../plotto/intercept_x86_64.S"
#elif defined(__aarch64__)
#include "../plotto/intercept_aarch64.S"
#else
#error "not supported"
#endif

.macro add_crep_warn name
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
        call_func crep_warn_call

        pop_caller_save
        call_func_ptr

        epilogue
        return
.end_\name:
        .size \name, .- \name
.endm

.macro add_crep_abort name
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
        call_func crep_abort_call

        pop_caller_save
        call_func_ptr

        epilogue
        return
.end_\name:
        .size \name, .- \name
.endm

#if 1
// general libc functions
//add_crep_warn rand
//add_crep_warn time

// pthreads
// add_crep_warn pthread_rwlock_wrlock
// add_crep_warn pthread_rwlock_rdlock
// add_crep_warn pthread_rwlock_unlock
// add_crep_warn pthread_yield
//add_crep_warn pthread_once
// pthread create and join
// add_crep_warn pthread_join
// add_crep_warn pthread_exit

add_crep_warn sem_timedwait
add_crep_warn sem_wait

// C++ std::thread create and join
add_crep_warn _ZNSt6thread15_M_start_threadESt10unique_ptrINS_6_StateESt14default_deleteIS1_EEPFvvE
add_crep_warn _ZNSt6thread4joinEv
add_crep_warn _ZNSt6thread6_StateD2Ev

// add_crep_warn  __tsan_init
// add_crep_warn  __libc_start_main

#else
add_crep_abort fread
#endif
