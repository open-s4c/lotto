// clang-format off
// RUN: (! %lotto %stress -- %b 2>&1)
// RUN: %lotto %show | %check %s
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

