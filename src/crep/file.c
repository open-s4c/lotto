/*
 */
#include <crep.h>
#include <stdio.h>
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/sys/logger_block.h>

#ifndef QLOTTO_ENABLED

CREP_FUNC(SIG(FILE *, fopen, const char *, path, const char *, mode),
          MAP(.retsize = sizeof(FILE *)))

CREP_FUNC(SIG(int, fclose, FILE *, stream), MAP(.retsize = sizeof(int)))

CREP_FUNC(SIG(size_t, fread, void *, ptr, size_t, size, size_t, nmemb, FILE *,
              stream),
          MAP(.retsize = sizeof(size_t),
              .params  = {
                   {.size = size * nmemb, .data = ptr},
                   NULL,
              }))

CREP_FUNC(SIG(size_t, fwrite, const void *, ptr, size_t, size, size_t, nmemb,
              FILE *, stream),
          MAP(.retsize = sizeof(size_t)))
#endif
