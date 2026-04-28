// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress --rusty-cas enable --memmgr-user uaf -- %b)
// CHECK: SEGV_ACCERR

// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

void
write_ptr(int *ptr, int val)
{
    *ptr = val;
}

int
main()
{
    write_ptr(NULL, 10);
    return 0;
}
