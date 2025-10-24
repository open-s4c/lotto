// clang-format off
// RUN: rm -f %s.replay.trace
// RUN: %lotto %stress -r 1 -o "" -- %b
// RUN: ! test -f %s.replay.trace
// clang-format on

#include <time.h>

int
main()
{
    time(NULL);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
