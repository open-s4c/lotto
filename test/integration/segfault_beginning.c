// clang-format off
// RUN: (! %lotto %stress -- %b)
// RUN: %lotto %show | filecheck %s
// CHECK: RECORD 2
// CHECK: reason:   SEGFAULT
// clang-format on

volatile int *x = 0;

int
main()
{
    *x = 0;
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
