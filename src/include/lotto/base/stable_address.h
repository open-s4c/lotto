/*
 */
#ifndef LOTTO_STABLE_ADDRESS_H
#define LOTTO_STABLE_ADDRESS_H

/*******************************************************************************
 * @file stable_address.h
 * @brief Map raw addresses to a value stable over multiple executions.
 *
 * Addresses are often affected by the address space randomization strategy and
 * other mechanisms of the operating system. This component maps raw addresses
 * to a value that is stable over multiple executions of the program. Stable
 * address offers a few methods to perform the mapping, ie, masking or proc
 * filesystem mapping.
 *
 ******************************************************************************/

#include <lotto/base/marshable.h>
#include <lotto/base/memory_map.h>
#include <lotto/util/macros.h>

#define FOR_EACH_STABLE_ADDRESS                                                \
    GEN_STABLE_ADDRESS(MASK)                                                   \
    GEN_STABLE_ADDRESS(MAP)

#define GEN_STABLE_ADDRESS(method) STABLE_ADDRESS_METHOD_##method,
typedef enum stable_address_method {
    STABLE_ADDRESS_METHOD_NONE = 0,
    FOR_EACH_STABLE_ADDRESS STABLE_ADDRESS_METHOD_END_,
    STABLE_ADDRESS_METHOD_SIZE_ = (1ULL << 62),
} stable_address_method_t;
#undef GEN_STABLE_ADDRESS

typedef struct stable_address {
    enum stable_address_type {
        ADDRESS_PTR,
        ADDRESS_MAP,
    } type;
    union stable_address_value {
        uintptr_t ptr;
#ifdef LOTTO_STABLE_ADDRESS_MAP
        map_address_t map;
#endif
    } value;
} stable_address_t;

#define STABLE_ADDRESS_MAX_LEN     256
#define STABLE_ADDRESS_MAX_STR_LEN 2048

/**
 * Parses a 0-terminated string and determines the corresponding stable address
 * method.
 *
 * @param str constant string, either NONE, MASK, or MAP
 * @return corresponding stable address method identifier
 */
stable_address_method_t stable_address_method_from(const char *str);

/**
 * Returns the string representation of the stable address method.
 *
 * @param m stable address method identifier
 * @return constant string, either NONE, MASK or MAP.
 */
const char *stable_address_method_str(stable_address_method_t m);

/**
 * Writes to a preallocated string all possible method value.
 *
 * @param s preallocated char array with at least
 * STABLE_ADDRESS_MAX_STR_LEN bytes.
 */
void stable_address_method_all_str(char *str);

/**
 * Converts given address to a stable address using selected method.
 * @param addr raw address
 * @param m stable address method
 * @return stable address
 */
stable_address_t stable_address_get(uintptr_t addr, stable_address_method_t m);

/**
 * Compares two stable addresses.
 * @param sa1 stable address
 * @param sa2 another stable address
 * @return true if addresses are the same, otherwise false
 *
 * Two addresses are the same if their type and value match.
 */
bool stable_address_equals(const stable_address_t *sa1,
                           const stable_address_t *sa2);

/**
 * Writes the address to a preallocated string.
 *
 * @param sa stable address
 * @param str preallocated char array with at least STABLE_ADDRESS_MAX_LEN
 * bytes.
 */
int stable_address_sprint(const stable_address_t *sa, char *s);

/**
 * Compares two stable addresses.
 *
 * @param sa stable address
 * @param sb stable address
 * @return <0 less, >0 greater, 0 equal
 */
int stable_address_compare(const stable_address_t *sa, const stable_address_t *sb);

#endif
