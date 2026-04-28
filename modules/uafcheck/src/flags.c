#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/sys/assert.h>

NEW_CALLBACK_FLAG(UAFCHECK_MAX_PAGES, "", "uafcheck-max-pages", "INT",
                  "number of freed pages kept protected by uafcheck",
                  flag_uval(UAFCHECK_DEFAULT_MAX_PAGES), {
                      ASSERT(as_uval(v) > 0);
                      uafcheck_config()->max_pages = as_uval(v);
                  })

NEW_CALLBACK_FLAG(UAFCHECK_PROB, "", "uafcheck-prob", "FLOAT",
                  "probability that an allocation is protected by uafcheck",
                  flag_dval(UAFCHECK_DEFAULT_PROB), {
                      ASSERT(as_dval(v) >= 0.0 && as_dval(v) <= 1.0);
                      uafcheck_config()->prob = as_dval(v);
                  })
