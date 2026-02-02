#ifndef LOTTO_HANDLER_ICHPT_H
#define LOTTO_HANDLER_ICHPT_H
#include <stdbool.h>
#include <stdint.h>

#include <lotto/sys/stream.h>

#define ICHPTS_FILE "ichpts.lotto"

bool is_ichpt(uintptr_t addr);
void add_ichpt(uintptr_t addr);
size_t ichpt_count();

void ichpt_save(stream_t *s);
void ichpt_load(stream_t *s, bool reset);

#endif
