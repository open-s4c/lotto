/*
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/flags.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

#define FLAGS_GET_PTR(CONST)                                                   \
    static CONST struct flag_val *_flags_get_##CONST##ptr(                     \
        CONST flags_t *flags, flag_t f)                                        \
    {                                                                          \
        ASSERT(f >= 1 && f < flags->nflags && "invalid flag value");           \
        return &flags->values[f];                                              \
    }

FLAGS_GET_PTR(const)
FLAGS_GET_PTR()

void
_flag_set_val(struct flag_val *flag_ptr, flag_t f, struct value val)
{
    flag_ptr->_val = val;
}

void
flags_set(flags_t *flags, flag_t f, struct value val, bool force_no_default,
          bool is_default)
{
    struct flag_val *flag_ptr = _flags_get_ptr(flags, f);
    ASSERT(flag_ptr);
    _flag_set_val(flag_ptr, f, val);
    flag_ptr->force_no_default = force_no_default;
    flag_ptr->is_default       = is_default;
}

struct flag_val
flags_get(const flags_t *flags, flag_t f)
{
    const struct flag_val *result_ptr = _flags_get_constptr(flags, f);
    return result_ptr ? *_flags_get_constptr(flags, f) : (struct flag_val){0};
}

enum value_type
flags_get_type(const flags_t *flags, flag_t f)
{
    struct flag_val v = flags_get(flags, f);
    return value_type(v._val);
}

struct value
flags_get_value(const flags_t *flags, flag_t f)
{
    return flag_value(flags_get(flags, f));
}

bool
flags_is_on(const flags_t *flags, flag_t f)
{
    return flag_is_on(flags_get(flags, f));
}

uint64_t
flags_get_uval(const flags_t *flags, flag_t f)
{
    return flag_as_uval(flags_get(flags, f));
}

const char *
flags_get_sval(const flags_t *flags, flag_t f)
{
    return flag_as_sval(flags_get(flags, f));
}

void
flags_set_default(flags_t *flags, flag_t f, struct value val)
{
    struct flag_val *flag_ptr = _flags_get_ptr(flags, f);
    ASSERT(flag_ptr);
    _flag_set_val(flag_ptr, f, val);
    flag_ptr->is_default = true;
}

void
flags_set_by_opt(flags_t *flags, flag_t f, struct value val)
{
    struct flag_val *flag_ptr = _flags_get_ptr(flags, f);
    ASSERT(flag_ptr);
    _flag_set_val(flag_ptr, f, val);
    flag_ptr->is_default = false;
}

flags_t *
flags_alloc(size_t nflags)
{
    flags_t *result =
        sys_malloc(sizeof(flags_t) + nflags * sizeof(struct flag_val));
    result->nflags = nflags;
    return result;
}

void
flags_free(flags_t *flags)
{
    sys_free(flags);
}

void
flags_cpy(flags_t *dst, const flags_t *src)
{
    ASSERT(dst->nflags >= src->nflags);
    sys_memcpy(dst->values, src->values, src->nflags * sizeof(struct flag_val));
}

static int
_flag_sel_find(flag_t *sel, flag_t b)
{
    int i = 0;
    while (sel[i] != FLAG_SEL_SENTINEL && sel[i] != b) {
        i++;
    }
    return i;
}

void
flag_sel_add(flag_t *sel, flag_t b)
{
    int i = _flag_sel_find(sel, b);
    if (sel[i] == FLAG_SEL_SENTINEL) {
        sel[i++] = b;
        sel[i]   = FLAG_SEL_SENTINEL;
    }
}
