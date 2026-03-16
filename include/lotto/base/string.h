/**
 * @file string.h
 * @brief Base declarations for string.
 */
#ifndef LOTTO_BASE_STRING_H
#define LOTTO_BASE_STRING_H

#include <stdint.h>

char *strupr(char *s);
uint64_t string_hash(const char *string);

#endif
