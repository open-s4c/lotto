/*
 * Minimal statemgr stub satisfying the engine build.
 */
#include <lotto/brokers/statemgr.h>

void
statemgr_register(marshable_t *m, state_type_t type)
{
    (void)m;
    (void)type;
}

size_t
statemgr_size(state_type_t type)
{
    (void)type;
    return 0;
}

const void *
statemgr_unmarshal(const void *buf, state_type_t type, bool publish)
{
    (void)type;
    (void)publish;
    return buf;
}

void *
statemgr_marshal(void *buf, state_type_t type)
{
    (void)type;
    return buf;
}

void
statemgr_print(state_type_t type)
{
    (void)type;
}

void
statemgr_record_unmarshal(const record_t *r)
{
    (void)r;
}
