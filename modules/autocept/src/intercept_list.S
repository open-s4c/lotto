.section .note.GNU-stack,"",@progbits

#if defined(__x86_64__)
#include "intercept_x86_64.S"
#elif defined(__aarch64__)
#include "intercept_aarch64.S"
#else
#error "not supported"
#endif

#include "intercept_generic.S"

// general libc functions
//add_interceptor rand
//add_interceptor time
//add_interceptor sleep

// pthreads
add_interceptor pthread_rwlock_wrlock
add_interceptor pthread_rwlock_rdlock
add_interceptor pthread_rwlock_unlock
add_interceptor pthread_yield
//add_interceptor pthread_once
// pthread create and join
//add_interceptor pthread_exit

add_interceptor sem_timedwait
add_interceptor sem_wait

add_interceptor epoll_wait
add_interceptor ioctl
#ifdef LOTTO_INTERCEPT_SYSCALL
add_interceptor syscall
#endif
add_interceptor __cxa_guard_acquire

// C++ std::thread create and join
// add_interceptor _ZNSt6thread15_M_start_threadESt10unique_ptrINS_6_StateESt14default_deleteIS1_EEPFvvE
// add_interceptor _ZNSt6thread4joinEv
// add_interceptor _ZNSt6thread6_StateD2Ev

// add_warn  __tsan_init
// add_warn  __libc_start_main
