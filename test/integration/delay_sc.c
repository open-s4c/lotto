// clang-format off
// XFAIL: *
// RUN: exit 1
// RUN: (! %lotto %sc --delayed-functions foo:0:1 --delayed-calls 2 --sc-exploration-rounds 50 --stable-address-method MAP -- %b 2>&1) | %check %s
// CHECK: delay_sc: {{.*}}/handler_capture_group.c:{{[0-9]+}}: _lib_destroy: Assertion `0 && "could not find a corresponding sequential state"' failed
// clang-format on

#include <pthread.h>
#include <stdatomic.h>

atomic_int x = 0;

void
foo()
{
    // The compiler generates the same code as if we woudl have
    //      int a = atomic_read(&x);
    //      atomic_store(&x, a+1);
    // This means we should observe different states if foo is atomic or not
    x = x + 1;
}

void *
thread()
{
    foo();
    long int y = x;
    return (void *)y;
}

int
main()
{
    x = 0;
    pthread_t thread1, thread2;
    pthread_create(&thread1, 0, thread, 0);
    pthread_create(&thread2, 0, thread, 0);
    pthread_join(thread1, 0);
    pthread_join(thread2, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
