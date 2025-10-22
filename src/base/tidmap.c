#include <lotto/base/tidmap.h>
#include <lotto/sys/assert.h>

void
tidmap_init(tidmap_t *map, marshable_t mi)
{
    map_init(map, mi);
}

tiditem_t *
tidmap_find(const tidmap_t *map, task_id id)
{
    ASSERT(id != NO_TASK);
    return map_find(map, id);
}

tiditem_t *
tidmap_find_first(const tidmap_t *map, bool(predicate)(const tiditem_t *))
{
    return map_find_first(map, predicate);
}

tiditem_t *
tidmap_register(tidmap_t *map, task_id id)
{
    ASSERT(id != NO_TASK);
    return map_register(map, id);
}

void
tidmap_deregister(tidmap_t *map, task_id id)
{
    ASSERT(id != NO_TASK);
    map_deregister(map, id);
}

size_t
tidmap_size(const tidmap_t *map)
{
    return map_size(map);
}

const tiditem_t *
tidmap_iterate(const tidmap_t *map)
{
    return map_iterate(map);
}

const tiditem_t *
tidmap_next(const tiditem_t *ti)
{
    return map_next(ti);
}

void
tidmap_clear(tidmap_t *map)
{
    map_clear(map);
}

tiditem_t *
tidmap_find_or_register(tidmap_t *tidmap, task_id id, tiditem_init_f init)
{
    ASSERT(id != NO_TASK);
    return map_find_or_register(tidmap, id, init);
}
