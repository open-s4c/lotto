/*
 */
/**
 * Macros for redirecting function calls to function pointers.
 */
#ifndef LOTTO_REDIRECT_H
#define LOTTO_REDIRECT_H

#define REDIRECT_VOID(prefix, name, proto, arg, ...)                           \
    void prefix##_##name proto                                                 \
    {                                                                          \
        arg->name(__VA_ARGS__);                                                \
    }
#define REDIRECT_RETURN(prefix, type, name, proto, arg, ...)                   \
    type prefix##_##name proto                                                 \
    {                                                                          \
        return arg->name(__VA_ARGS__);                                         \
    }

#define REDIRECT_CAST_VOID(prefix, name, proto, arg_type, arg, ...)            \
    void prefix##_##name proto                                                 \
    {                                                                          \
        ((arg_type *)arg)->name(__VA_ARGS__);                                  \
    }
#define REDIRECT_CAST_RETURN(prefix, type, name, proto, arg_type, arg, ...)    \
    type prefix##_##name proto                                                 \
    {                                                                          \
        return ((arg_type *)arg)->name(__VA_ARGS__);                           \
    }

#endif
