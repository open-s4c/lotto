#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>

NEW_CALLBACK_FLAG(UAFCHECK_MAX_PAGES, "", "uafcheck-max-pages", "INT",
                  "number of freed pages kept protected by uafcheck",
                  flag_uval(UAFCHECK_DEFAULT_MAX_PAGES), {
                      ASSERT(as_uval(v) > 0);
                      uafcheck_config()->max_pages = as_uval(v);
                  })

NEW_CALLBACK_FLAG(UAFCHECK_PROB, "", "uafcheck-prob", "FLOAT",
                  "probability that an allocation is protected by uafcheck",
                  flag_dval(UAFCHECK_DEFAULT_PROB), {
                      double prob = as_dval(v);
                      if (prob < 0.0 || prob > 1.0) {
                          sys_fprintf(stderr,
                                      "error: --uafcheck-prob must be in range "
                                      "[0, 1]: %g\n",
                                      prob);
                          sys_exit(1);
                      }
                      uafcheck_config()->prob = as_dval(v);
                  })
