/*
 */
#include <lotto/base/category.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

#define GEN_CAT(cat) [CAT_##cat] = #cat,
static const char *_base_category_map[] = {FOR_EACH_CATEGORY};
#undef GEN_CAT

BIT_STR(base_category, CAT)
