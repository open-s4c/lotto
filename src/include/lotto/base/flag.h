#ifndef LOTTO_RUNTIME_FLAG_H
#define LOTTO_RUNTIME_FLAG_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/value.h>
#include <lotto/util/macros.h>

typedef unsigned int flag_t;

#define FLAG_NONE         0
#define FLAG_SEL_SENTINEL (-1U)

struct flag_val {
    struct value _val;
    bool is_default;
    bool force_no_default;
};

#define flag_value(f)   ((f)._val)
#define flag_type(f)    value_type(flag_value(f))
#define flag_is_on(f)   is_on(flag_value(f))
#define flag_as_uval(f) as_uval(flag_value(f))
#define flag_as_sval(f) as_sval(flag_value(f))

/* default flags for
    .is_default = true
    .force_no_default = false
*/
#define flag_sval(v)                                                           \
    (struct flag_val)                                                          \
    {                                                                          \
        ._val       = {._type = VALUE_TYPE_STRING, ._sval = (v)},              \
        .is_default = true, .force_no_default = false                          \
    }
#define flag_uval(v)                                                           \
    (struct flag_val)                                                          \
    {                                                                          \
        ._val       = {._type = VALUE_TYPE_UINT64, ._uval = (v)},              \
        .is_default = true, .force_no_default = false                          \
    }
#define flag_bool(v)                                                           \
    (struct flag_val)                                                          \
    {                                                                          \
        ._val = {._type = VALUE_TYPE_BOOL, ._bval = (v)}, .is_default = true,  \
        .force_no_default = false                                              \
    }


/* variant flags for enforcing an argument from command line options
    .is_default = true
    .force_no_default = false
*/
#define flag_sval_force_opt(v)                                                 \
    (struct flag_val)                                                          \
    {                                                                          \
        ._val       = {._type = VALUE_TYPE_STRING, ._sval = (v)},              \
        .is_default = true, .force_no_default = true                           \
    }
#define flag_uval_force_opt(v)                                                 \
    (struct flag_val)                                                          \
    {                                                                          \
        ._val       = {._type = VALUE_TYPE_UINT64, ._uval = (v)},              \
        .is_default = true, .force_no_default = true                           \
    }

#define flag_on()  flag_bool(true)
#define flag_off() flag_bool(false)

#endif
