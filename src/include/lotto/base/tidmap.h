#ifndef LOTTO_TIDMAP_H
#define LOTTO_TIDMAP_H

#include <lotto/base/map.h>
#include <lotto/base/task_id.h>

typedef mapitem_t tiditem_t;
typedef map_t tidmap_t;
typedef mapitem_init_f tiditem_init_f;

/**
 * Initializes the task tidmap with no tasks.
 *
 * @param map tidmap to initialize
 * @param mi marshable interface of tiditem
 */
void tidmap_init(tidmap_t *map, marshable_t mi);

/**
 * Finds task with given id.
 *
 * @param map tidmap object
 * @param id task id
 * @return task object matching the id or NULL if task not found
 */
tiditem_t *tidmap_find(const tidmap_t *map, task_id id);

/**
 * Finds the first item satisfying the predicate.
 *
 * @param map tidmap object
 * @param predicate task id
 * @return task object matching the predicate or NULL if task not found
 */
tiditem_t *tidmap_find_first(const tidmap_t *map,
                             bool(predicate)(const tiditem_t *));

/**
 * Register a new task in the tidmap.
 *
 * The task has to have a unique id within the tidmap.
 *
 * @param map tidmap object
 * @param id task id to register
 * @return new task object
 */
tiditem_t *tidmap_register(tidmap_t *map, task_id id);

/**
 * Deregister task from tidmap.
 *
 * @param map tidmap object
 * @param id task id to deregister
 */
void tidmap_deregister(tidmap_t *map, task_id id);

/**
 * Returns the number of items in the tidmap.
 *
 * @param map tidmap object
 * @return number of items in the map
 */
size_t tidmap_size(const tidmap_t *map);

/**
 * Returns the first item in the tidmap.
 *
 * @param map tidmap object
 * @return first item in map
 */
const tiditem_t *tidmap_iterate(const tidmap_t *map);

/**
 * Returns the next item in the tidmap.
 *
 * @param ti current item
 * @return next item, or NULL if no item left
 */
const tiditem_t *tidmap_next(const tiditem_t *ti);

/**
 * Removes all elements from the tidmap.
 *
 * @param map tidmap object
 */
void tidmap_clear(tidmap_t *map);

/**
 * Finds task with given id or registers it if it is not present.
 *
 * @param map tidmap object
 * @param id task id
 * @param init element initializer
 * @return task object matching the id
 */
tiditem_t *tidmap_find_or_register(tidmap_t *tidmap, task_id id,
                                   tiditem_init_f init);

#endif
