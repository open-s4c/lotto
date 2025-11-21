/*
 */
#ifndef LOTTO_RUNTIME_FLAGS_H
#define LOTTO_RUNTIME_FLAGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lotto/base/flag.h>

typedef struct {
    size_t nflags;
    struct flag_val values[];
} flags_t;

enum value_type flags_get_type(const flags_t *flags, flag_t f);

bool flags_is_on(const flags_t *flags, flag_t f);
uint64_t flags_get_uval(const flags_t *flags, flag_t f);
const char *flags_get_sval(const flags_t *flags, flag_t f);
struct value flags_get_value(const flags_t *flags, flag_t f);
struct flag_val flags_get(const flags_t *flags, flag_t f);

void flags_set_default(flags_t *flags, flag_t f, struct value val);
void flags_set_by_opt(flags_t *flags, flag_t f, struct value val);

void flags_set(flags_t *flags, flag_t f, struct value val,
               bool force_no_default, bool is_default);

void flags_init(flags_t *flags);
flags_t *flags_alloc(size_t nflags);
void flags_cpy(flags_t *dst, const flags_t *src);
void flags_free(flags_t *flags);

void flag_sel_add(flag_t *sel, flag_t b);

#endif
