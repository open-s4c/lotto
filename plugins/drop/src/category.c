/*
 */
#include <category.h>

#include <lotto/brokers/catmgr.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

static category_t _cat;

static void LOTTO_CONSTRUCTOR
_init(void)
{
    _cat = new_category("CAT_DROP");
}

category_t
drop_category()
{
    ASSERT(_cat);
    return _cat;
}
