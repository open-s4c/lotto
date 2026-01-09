/*
 */
#ifndef LOTTO_VALUE_H
#define LOTTO_VALUE_H

#include <stdbool.h>
#include <stdint.h>

enum value_type {
    VALUE_TYPE_NONE,
    VALUE_TYPE_BOOL,
    VALUE_TYPE_UINT64,
    VALUE_TYPE_STRING,
    VALUE_TYPE_ANY
};

struct value {
    enum value_type _type;
    union {
        bool _bval;
        uint64_t _uval;
        const char *_sval;
        const void *_any;
        uint8_t _u8val;
    };
};

#define value_type(v) ((v)._type)

#define nil ((struct value){0})

#define on()  bval(true)
#define off() bval(false)
#define bval(v)                                                                \
    (struct value)                                                             \
    {                                                                          \
        ._type = VALUE_TYPE_BOOL, ._bval = (v)                                 \
    }
#define uval(v)                                                                \
    (struct value)                                                             \
    {                                                                          \
        ._type = VALUE_TYPE_UINT64, ._uval = (v)                               \
    }
#define sval(v)                                                                \
    (struct value)                                                             \
    {                                                                          \
        ._type = VALUE_TYPE_STRING, ._sval = (v)                               \
    }
#define any(v)                                                                 \
    (struct value)                                                             \
    {                                                                          \
        ._type = VALUE_TYPE_ANY, ._any = (v)                                   \
    }

#define is_on(v)                                                               \
    ({                                                                         \
        ASSERT((v)._type == VALUE_TYPE_BOOL);                                  \
        (v)._bval;                                                             \
    })

#define is_off(v)                                                              \
    ({                                                                         \
        ASSERT((v)._type == VALUE_TYPE_BOOL);                                  \
        !(v)._bval;                                                            \
    })
#define as_uval(v)                                                             \
    ({                                                                         \
        ASSERT((v)._type == VALUE_TYPE_UINT64);                                \
        (v)._uval;                                                             \
    })
#define as_sval(v)                                                             \
    ({                                                                         \
        ASSERT((v)._type == VALUE_TYPE_STRING);                                \
        (v)._sval;                                                             \
    })
#define as_any(v)                                                              \
    ({                                                                         \
        ASSERT((v)._type == VALUE_TYPE_ANY);                                   \
        (v)._any;                                                              \
    })

#endif
