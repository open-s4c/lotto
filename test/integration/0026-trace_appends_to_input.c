// clang-format off
// RUN: rm -f %s.base.trace %s.next.trace
// RUN: %lotto start -t %T -o %s.base.trace -- %b
// RUN: %lotto config -t %T -i %s.base.trace -o %s.next.trace
// RUN: %lotto show -t %T -i %s.next.trace | %check %s
// CHECK: kind:     START
// CHECK: kind:     CONFIG
// CHECK: kind:     CONFIG
// clang-format on

int
main(void)
{
    return 0;
}
