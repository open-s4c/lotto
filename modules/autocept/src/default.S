#include <lotto/modules/autocept.h>

// general libc functions
ADD_INTERCEPTOR(select)
ADD_INTERCEPTOR(epoll_wait)
ADD_INTERCEPTOR(recv)
//ADD_INTERCEPTOR(send)
ADD_INTERCEPTOR(accept)
//ADD_INTERCEPTOR(listen)
//ADD_INTERCEPTOR(ioctl)

// C++ std::thread create and join
//ADD_INTERCEPTOR(_ZNSt6thread15_M_start_threadESt10unique_ptrINS_6_StateESt14default_deleteIS1_EEPFvvE)
//ADD_INTERCEPTOR(_ZNSt6thread4joinEv)
//ADD_INTERCEPTOR(_ZNSt6thread6_StateD2Ev)
