// clang-format off
// RUN: rm -f %s.replay.trace
// RUN: %lotto record -t %T -o "" -- %b | %check %s
// RUN: ! test -f %s.replay.trace
// CHECK: RECORD_NO_OUTPUT_OK
// clang-format on

#include <stdio.h>

int
main(void)
{
    printf("RECORD_NO_OUTPUT_OK\n");
    return 0;
}
