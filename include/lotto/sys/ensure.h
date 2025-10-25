#ifndef LOTTO_ENSURE_H
#define LOTTO_ENSURE_H

#include <lotto/sys/abort.h>

#define ENSURE(cond)                                                           \
    do {                                                                       \
        if (!(cond)) {                                                         \
            sys_assert_fail(#cond, __FILE__, (unsigned int)__LINE__,           \
                            __FUNCTION__);                                     \
        }                                                                      \
    } while (0)

#endif /* LOTTO_ENSURE_H */
