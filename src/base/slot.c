#include <lotto/base/slot.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

#define GEN_SLOT(slot) [SLOT_##slot] = #slot,
static const char *_slot_map[] = {FOR_EACH_SLOT};
#undef GEN_SLOT

BIT_STR(slot, SLOT)
