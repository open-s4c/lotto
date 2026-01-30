#include <stdbool.h>

#include <lotto/runtime/intercept.h>

int
lotto_yield(bool advisory)
{
    intercept_capture(ctx(.func = __FUNCTION__,
                          .cat  = advisory ? CAT_SYS_YIELD : CAT_USER_YIELD));
    return 0;
}
