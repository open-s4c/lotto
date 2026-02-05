/*
 */
#include <string.h>

#include <lotto/base/stable_address.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>

#define MASK 0xFFF

#define GEN_STABLE_ADDRESS(mode) [STABLE_ADDRESS_METHOD_##mode] = #mode,
static const char *_stable_address_method_map[] = {
    [STABLE_ADDRESS_METHOD_NONE] = "NONE",
    FOR_EACH_STABLE_ADDRESS};
#undef GEN_STABLE_ADDRESS

BIT_FROM(stable_address_method, STABLE_ADDRESS_METHOD)
BIT_STR(stable_address_method, STABLE_ADDRESS_METHOD)
BIT_ALL_STR(stable_address_method, STABLE_ADDRESS_METHOD)

stable_address_t
stable_address_get(uintptr_t addr, stable_address_method_t method)
{
#ifdef LOTTO_STABLE_ADDRESS_MAP
    stable_address_t sa;
#endif
    switch (method) {
        case STABLE_ADDRESS_METHOD_NONE:
            return (stable_address_t){.type = ADDRESS_PTR, .value.ptr = addr};
        case STABLE_ADDRESS_METHOD_MASK:
            return (stable_address_t){.type      = ADDRESS_PTR,
                                      .value.ptr = addr & MASK};
#ifdef LOTTO_STABLE_ADDRESS_MAP
        case STABLE_ADDRESS_METHOD_MAP: {
            sa = (stable_address_t){.type = ADDRESS_MAP};
            memory_map_address_lookup(addr, &sa.value.map);
            return sa;
        }
#endif
        default:
            ASSERT(0 && "unknown stable address method");
            return (stable_address_t){};
    }
}

bool
stable_address_equals(const stable_address_t *sa1, const stable_address_t *sa2)
{
    ASSERT(sa1->type == sa2->type);
    switch (sa1->type) {
        case ADDRESS_PTR:
            return sa1->value.ptr == sa2->value.ptr;
#ifdef LOTTO_STABLE_ADDRESS_MAP
        case ADDRESS_MAP:
            return memory_map_address_equals(&sa1->value.map, &sa2->value.map);
#endif
        default:
            ASSERT(0 && "unknown stable address type");
            return false;
    }
}

int
stable_address_sprint(const stable_address_t *sa, char *output)
{
    switch (sa->type) {
        case ADDRESS_PTR:
            return sys_sprintf(output, "0x%016lx", sa->value.ptr);
#ifdef LOTTO_STABLE_ADDRESS_MAP
        case ADDRESS_MAP:
            return memory_map_address_sprint(&sa->value.map, output);
#endif
        default:
            ASSERT(0 && "unknown stable address type");
            return -1;
    }
}


int
stable_address_compare(const stable_address_t *sa, const stable_address_t *sb)
{
    ASSERT(sa->type == sb->type);
    switch(sa->type) {
        case ADDRESS_PTR:
            return sa->value.ptr < sb->value.ptr ? -1 :
                   sa->value.ptr > sb->value.ptr ? 1 :
                   0;
        case ADDRESS_MAP: {
            int name_cmp = strcmp(sa->value.map.name, sb->value.map.name);
            if (name_cmp != 0)
                return name_cmp;
            return sa->value.map.offset < sb->value.map.offset ? -1 :
                   sa->value.map.offset > sb->value.map.offset ? 1 :
                   0;
        }
        default:
            ASSERT(false && "unreachable");
            return 0;
    }
}
