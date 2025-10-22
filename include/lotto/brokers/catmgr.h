#ifndef LOTTO_CAT_MGR_H
#define LOTTO_CAT_MGR_H

#include <lotto/base/category.h>

category_t new_category(const char *name);
const char *category_str(category_t category);

#endif
