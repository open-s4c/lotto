#ifndef LOTTO_VEC_H
#define LOTTO_VEC_H

/*******************************************************************************
 * @file vec.h
 * @brief Marshable dynamic array of fixed-size items
 *
 * Vec manages the memory allocation of the fixed-size items and can be
 * marshaled/unmarshaled; under the hood it is a dynamic array.
 *
 * Vec supports insertion in amortized O(1) time, but does not support
 * fast deletion.
 */

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct vecitem {
    marshable_t m;
    char payload[0];
} vecitem_t;

typedef struct vec {
    marshable_t m;    //< vec marshable interface
    marshable_t mi;   //< item marshable interface
    size_t cnt;       //< count of items in vec
    size_t cap;       //< capacity
    vecitem_t *items; //< buffer of the items
} vec_t;

typedef int (*vecitem_comparator_f)(const vecitem_t *, const vecitem_t *);

/**
 * Initializes vec with no items.
 *
 * @param v vec object to initialize.
 * @param mi marshable interface of items.
 */
void vec_init(vec_t *v, marshable_t mi);

/**
 * Allocates and adds an item to the vec.
 *
 * @param v vec object
 * @return new item
 */
vecitem_t *vec_add(vec_t *v);

/**
 * Reserves at least cap elements.
 *
 * @param cap requested capacity
 */
void vec_reserve(vec_t *v, size_t cap);

/**
 * Returns the number of items in the vec.
 *
 * @param v vec object
 * @return number of items in the vec
 */
size_t vec_size(const vec_t *v);


/**
 * Gets an item.
 *
 * @param v vec object
 * @param i index
 */
vecitem_t *vec_get(vec_t *v, size_t i);


/**
 * Removes all elements from the vec.
 *
 * @param v vec object
 */
void vec_clear(vec_t *v);


/**
 * Sorts all elements in the vec.
 *
 * @param v vec object
 * @param compar comparator
 */
void vec_sort(vec_t *v, vecitem_comparator_f compar);


/**
 * Appends all elements from right into left.
 *
 * @param left vec object
 * @param right vec object
 */
void vec_concat(vec_t *left, const vec_t *right);


/**
 * Sets left to be the union of left and right.
 *
 * @param left vec object
 * @param right vec object
 * @param compar comparator
 */
void vec_union(vec_t *left, const vec_t *right, vecitem_comparator_f compar);


#endif
