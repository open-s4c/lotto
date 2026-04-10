#include <lotto/modules/autocept.h>

// general libc functions
ADD_INTERCEPTOR(accept)
ADD_INTERCEPTOR(epoll_wait)
ADD_INTERCEPTOR(recv)
//ADD_INTERCEPTOR(ioctl)
//ADD_INTERCEPTOR(rand)
//ADD_INTERCEPTOR(time)
//ADD_INTERCEPTOR(sleep)

//ADD_INTERCEPTOR(sem_timedwait)
//ADD_INTERCEPTOR(sem_wait)

//ADD_INTERCEPTOR(__cxa_guard_acquire)

// C++ std::thread create and join
//ADD_INTERCEPTOR(_ZNSt6thread15_M_start_threadESt10unique_ptrINS_6_StateESt14default_deleteIS1_EEPFvvE)
//ADD_INTERCEPTOR(_ZNSt6thread4joinEv)
//ADD_INTERCEPTOR(_ZNSt6thread6_StateD2Ev)
