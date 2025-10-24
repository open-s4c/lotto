// clang-format off
// RUN: !( timeout 5 %lotto %stress -r 1 )
// clang-format on

#include <lotto/order.h>

int
main()
{
    lotto_order(2);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
