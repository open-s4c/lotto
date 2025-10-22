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

#define once(CODE)                                                             \
    do {                                                                       \
        static bool _initd = false;                                            \
        if (!_initd) {                                                         \
            _initd = true;                                                     \
            (CODE);                                                            \
        }                                                                      \
    } while (0)

#endif
