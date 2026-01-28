/*
 */
#include <stdbool.h>
#include <stdint.h>

#include <lotto/runtime/intercept.h>
#include <lotto/sys/assert.h>
#include <lotto/unsafe/disable.h>
#include <lotto/util/macros.h>

void
_lotto_disable()
{
    mediator_detach(get_mediator(false));
}

void
_lotto_enable()
{
    mediator_attach(get_mediator(false));
}

bool
_lotto_disabled()
{
    return mediator_detached(get_mediator(false));
}
