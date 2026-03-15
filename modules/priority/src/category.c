#include "category.h"
#include "lotto/core/driver/events.h"
#include <lotto/engine/catmgr.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

static category_t _cat;

ON_REGISTRATION_PHASE({ _cat = new_category("CAT_PRIORITY"); })

category_t
priority_category()
{
    ASSERT(_cat);
    return _cat;
}
