#include <lotto/base/marshable.h>
#include <lotto/base/record.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/stdlib.h>

typedef struct {
    marshable_t m;
    unsigned value;
} state_t;

int
main(void)
{
    state_t seed = {.m = MARSHABLE_STATIC(sizeof(state_t)), .value = 123};
    state_t constraint = {.m = MARSHABLE_STATIC(sizeof(state_t)), .value = 7};
    statemgr_register(901, &seed.m, STATE_TYPE_CONFIG);
    statemgr_register(902, &constraint.m, STATE_TYPE_CONFIG);
    record_t *full = record_alloc(statemgr_size(STATE_TYPE_CONFIG));
    full->kind = RECORD_CONFIG;
    full->clk = 42;
    statemgr_marshal(full->data, STATE_TYPE_CONFIG);

    record_t *partial = statemgr_config_record_for_slot(full, 902);
    ENSURE(partial && partial->clk == 42 && partial->kind == RECORD_CONFIG);
    ENSURE(partial->size < full->size);
    seed.value = 456;
    constraint.value = 0;
    statemgr_record_unmarshal(partial);
    ENSURE(seed.value == 456); /* Do not reset the live random generator. */
    ENSURE(constraint.value == 7);
    ENSURE(statemgr_config_record_for_slot(full, 903) == NULL);
    sys_free(partial);
    sys_free(full);
    return 0;
}
