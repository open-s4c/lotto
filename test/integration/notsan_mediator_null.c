// clang-format off
// UNSUPPORTED: aarch64
// RUN: (! %lotto %run -- %b 2>&1) | %check %s
// CHECK: [{{[[:digit:]]+}}](estimated) SIGABRT
// clang-format on
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int
main()
{
    raise(SIGABRT);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR
