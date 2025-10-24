#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/base/memory_map.h>

void
function_test()
{
    map_address_t addr;
    memory_map_address_lookup((uintptr_t)function_test, &addr);
    assert(strstr(addr.name, "test/unit/engine/util/memory_map_test"));
}

void
heap_test()
{
    map_address_t addr;
    memory_map_address_lookup((uintptr_t)malloc(1), &addr);
    assert(strcmp(addr.name, "[heap]") == 0);
}

void
stack_test()
{
    char x;
    map_address_t addr;
    memory_map_address_lookup((uintptr_t)&x, &addr);
    assert(strcmp(addr.name, "[stack]") == 0);
}

int
main()
{
    void (*tests[])() = {function_test, heap_test, stack_test};
    for (size_t i = 0; i < sizeof tests / sizeof(void (*)(void)); i++) {
        tests[i]();
    }
    return 0;
}
