/*
 */

#include <lotto/await.h>
#include <lotto/runtime/intercept.h>

// defined in Rust code
category_t await_cat();

// strong definition
void
_lotto_await(void *addr)
{
    intercept_capture(
        ctx(.func = __func__, .cat = await_cat(), .args = {arg_ptr(addr)}));
}
