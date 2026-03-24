// clang-format off
// UNSUPPORTED: aarch64
// RUN: (! %lotto %run -- %b 2>&1) | %check %s
// CHECK: [{{[[:digit:]]+}}] SIGABRT
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

