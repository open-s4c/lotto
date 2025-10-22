// clang-format off
// UNSUPPORTED: aarch64
// RUN: %lotto %trace -- %b
// RUN: ( LD_PRELOAD=%t/libruntime.so:%t/libplotto.so:%t/libengine.so LD_LIBRARY_PATH=%t LOTTO_TEMP_DIR=%t LOTTO_LOG_LEVEL=debug LOTTO_RECORDER_TYPE=flat %b || true ) 2>&1 | %check
// CHECK: CAPTURE
// clang-format on

#include <stdio.h>

int
main(void)
{
    puts("CAPTURE");
    return 0;
}
