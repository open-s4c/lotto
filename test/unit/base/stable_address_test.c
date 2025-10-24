#include <stdlib.h>

#include <lotto/base/memory_map.h>
#include <lotto/base/stable_address.h>
#include <lotto/sys/ensure.h>

#define MASK 0xFFF

void
test_ptr()
{
    uintptr_t ptr = (uintptr_t)malloc(10);

    stable_address_t stable_addr_ptr;
    stable_addr_ptr.type      = ADDRESS_PTR;
    stable_addr_ptr.value.ptr = ptr;

    // _method_from(): method NONE
    // _address_get(): method NONE
    stable_address_t tmp =
        stable_address_get(ptr, stable_address_method_from("NONE"));
    ENSURE(tmp.type == ADDRESS_PTR);
    // _equals(): type PTR
    ENSURE(stable_address_equals(&stable_addr_ptr, &tmp));

    // _method_from(): method MASK
    // _address_get(): method MASK
    tmp = stable_address_get(ptr, stable_address_method_from("MASK"));
    ENSURE((ptr & MASK) == tmp.value.ptr);
    free((void *)ptr);
}

void
test_map()
{
    void *ptr = malloc(10);

    // _method_from(): method MAP
    // _address_get(): method MAP
    stable_address_get((uintptr_t)ptr, stable_address_method_from("MAP"));

    // _stable_address_size: type MAP
    map_address_t map_addr;
    memory_map_address_lookup((uintptr_t)ptr, &map_addr);
    size_t len = map_addr.name[0] ? strlen(map_addr.name) + 1 : 0;
    ENSURE(len > 0);
    free(ptr);
}

int
main()
{
    test_ptr();
    test_map();
    return 0;
}
