#ifndef LOTTO_MAP_H
#define LOTTO_MAP_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct mapitem {
    marshable_t m;
    struct mapitem *next;
    char payload[0];
    uint64_t key;
} mapitem_t;


typedef struct map {
    marshable_t m;   //< map marshable interface
    marshable_t mi;  //< item marshable interface
    char payload[0]; //< marker of marshable payload
    mapitem_t *tail; //< tail of item list
    size_t nitems;   //< number of items in map
} map_t;

typedef void (*mapitem_init_f)(mapitem_t *);

/**
 * Initializes the map with no elements.
 *
 * @param map map to initialize
 * @param mi  marshable interface of mapitem
 */
void map_init(map_t *map, marshable_t mi);

/**
 * Finds item with given key.
 *
 * @param  map map object
 * @param  key key
 * @return item object matching the key or NULL if item not found
 */
mapitem_t *map_find(const map_t *map, uint64_t key);

/**
 * Finds the first item satisfying the predicate.
 *
 * @param map       map object
 * @param predicate predicate
 * @return          item object matching the predicate or NULL if item not found
 */
mapitem_t *map_find_first(const map_t *map, bool(predicate)(const mapitem_t *));

/**
 * Register a new item in the map.
 *
 * The item has to have a unique key within the map.
 *
 * @param map map object
 * @param key item key to register
 * @return    new item object
 */
mapitem_t *map_register(map_t *map, uint64_t key);

/**
 * Deregister item from map.
 *
 * @param map map object
 * @param key item key to deregister
 */
void map_deregister(map_t *map, uint64_t key);

/**
 * Returns the number of items in the map.
 *
 * @param  map map object
 * @return number of items in the map
 */
size_t map_size(const map_t *map);

/**
 * Returns the first item in the map.
 *
 * @param  map map object
 * @return first item in map
 */
const mapitem_t *map_iterate(const map_t *map);

/**
 * Returns the next item in the map.
 *
 * @param mi current item
 * @return next item, or NULL if no item left
 */
const mapitem_t *map_next(const mapitem_t *mi);

/**
 * Removes all elements from the map.
 *
 * @param map map object
 */
void map_clear(map_t *map);

/**
 * Finds item with given key or registers it if it is not present.
 *
 * @param  map map object
 * @param  key key
 * @param  init element initializer
 * @return item object matching the key
 */
mapitem_t *map_find_or_register(map_t *map, uint64_t key, mapitem_init_f init);

#endif
