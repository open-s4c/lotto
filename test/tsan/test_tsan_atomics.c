// a file to check if our tsan atomic interceptor follows the interface.

// Include the official tsan interface
#include "llvm_15_0_4/include/tsan_interface_atomic.h"

// Include our implementation
#define TSANO_MODE 1
#include "../../src/plotto/intercept_tsan.c" // NOLINT(bugprone-suspicious-include)

void
runtime_ingress(capture_point *ctx, metadata_t *md)
{
    (void)ctx;
    (void)md;
}

int
main(int argc, char const *argv[])
{
    return 0;
}
