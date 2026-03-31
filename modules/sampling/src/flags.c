#include "parser.h"
#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

static void
_report_parse_error(size_t line, const char *msg, const char *detail, void *arg)
{
    const char *fname = arg != NULL ? (const char *)arg : "<sampling-config>";
    logger_errorf("%s:%zu: %s: %s\n", fname, line, msg, detail);
}

static char *
_load_config_text(const char *fname)
{
    FILE *fp;
    size_t len;
    char *text;
    size_t read_items;

    fp = fopen(fname, "r");
    if (fp == NULL) {
        logger_debugf("failed to open sampling configuration: %s\n", fname);
        return NULL;
    }

    sys_fseek(fp, 0, SEEK_END);
    len  = sys_ftell(fp);
    text = sys_malloc(len + 1);
    sys_rewind(fp);
    read_items = sys_fread(text, len, 1, fp);
    ASSERT(read_items == 1 && "reading failed");
    fclose(fp);
    text[len] = '\0';
    return text;
}

NEW_CALLBACK_FLAG(SAMPLING_CONFIG, "", "sampling-config", "FILE",
                  "sampling configuration file (containing key=value lines)",
                  flag_sval("sampling.conf"), {
                      char *text;

                      ASSERT(sys_strlen(as_sval(v)) < PATH_MAX);
                      strcpy(sampling_config()->filename, as_sval(v));

                      text = _load_config_text(sampling_config()->filename);
                      if (text == NULL)
                          return;

                      sampling_parse_config(
                          text, sampling_entries(), _report_parse_error,
                          (void *)sampling_config()->filename);
                      sys_free(text);
                  })
