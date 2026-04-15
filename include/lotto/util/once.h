/**
 * @file once.h
 * @brief Utility declarations for once.
 */
#ifndef LOTTO_ONCE_H
#define LOTTO_ONCE_H

#include <stdbool.h>

#define ONCE()                                                                 \
    do {                                                                       \
        static bool _initd = false;                                            \
        if (_initd)                                                            \
            return;                                                            \
        _initd = true;                                                         \
    } while (0)

#define once(...)                                                              \
    do {                                                                       \
        static bool _initd = false;                                            \
        if (!_initd) {                                                         \
            _initd = true;                                                     \
            __VA_ARGS__;                                                       \
        }                                                                      \
    } while (0)

#endif
