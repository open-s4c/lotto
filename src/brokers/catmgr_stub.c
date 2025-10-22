/*
 * Minimal category manager stub.
 */
#include <lotto/brokers/catmgr.h>

category_t
new_category(const char *name)
{
    (void)name;
    return CAT_END_;
}

const char *
category_str(category_t category)
{
    if (category < CAT_END_) {
        return base_category_str(category);
    }
    return "UNKNOWN";
}
