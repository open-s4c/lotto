/*
 */

#include <lotto/await_while.h>
#include <lotto/runtime/intercept.h>

// defined in Rust code
category_t spin_start_cat();
category_t spin_end_cat();

// strong definition
void
_lotto_spin_start()
{
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = spin_start_cat(), .args = {}));
}

void
_lotto_spin_end(uint32_t cond)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = spin_end_cat(),
                          .args = {arg(uint32_t, cond)}));
}
