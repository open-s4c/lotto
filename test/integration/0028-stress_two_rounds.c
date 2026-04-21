// clang-format off
// RUN: rm -f %s.replay.trace
// RUN: %lotto stress -t %T -r 2 -v -o %s.replay.trace -- %b | %check %s
// RUN: test -s %s.replay.trace
// CHECK: [lotto] round: 0/2
// CHECK: [lotto] round: 1/2
// clang-format on

int
main(void)
{
    return 0;
}
