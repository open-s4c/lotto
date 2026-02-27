#include <stdlib.h>

#include <lotto/sys/memmgr_user.h>
#define FORWARD(F, ...) CONCAT(memmgr_user_, F)(__VA_ARGS__)

void
func_c()
{
    void *addr_1 = FORWARD(alloc, 10);
    void *addr_2 = FORWARD(alloc, 10);
    (void)addr_2;

    FORWARD(free, addr_1);
}

void
func_b()
{
    void *addr_3 = FORWARD(alloc, 10);
    void *addr_4 = FORWARD(alloc, 10);
    (void)addr_4;

    FORWARD(free, addr_3);
}

void
func_a()
{
    func_b();
}

int
main(void)
{
    func_a();
    return 0;
}
