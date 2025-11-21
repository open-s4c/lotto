/*
 */
#ifndef LOTTO_BAG_H
#define LOTTO_BAG_H
/*******************************************************************************
 * @file bag.h
 * @brief Marshabled bag of fixed-size items
 *
 * Bag manages the memory allocation of the fixed-size items and can be
 * marshaled/unmarshaled.
 ******************************************************************************/
#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct bagitem {
    marshable_t m;
    struct bagitem *next;
    char payload[0];
} bagitem_t;

typedef struct bag {
    marshable_t m;   //< bag marshable interface
    marshable_t mi;  //< item marshable interface
    char payload[0]; //< marker of marshable payload
    bagitem_t *tail; //< tail of task list
    size_t nitems;   //< number of items in bag
} bag_t;

/**
 * Initializes bag with no items.
 *
 * @param b bag object to initialize
 * @param mi marshable interface of items
 */
void bag_init(bag_t *b, marshable_t mi);

/**
 * Allocates and adds a item to the bag.
 *
 * @param b bag object
 * @return new item
 */
bagitem_t *bag_add(bag_t *b);

/**
 * Remove item from bag and frees item.
 *
 * @param b bag oject
 * @param i bag item to remove
 */
void bag_remove(bag_t *b, bagitem_t *i);


/**
 * Returns the number of items in the bag.
 *
 * @param b bag oject
 * @return number of items in the bag
 */
size_t bag_size(const bag_t *b);

/**
 * Returns one item in the bag to be used as iterator.
 *
 * @param b bag oject
 * @return one item in the bag or NULL if empty
 */
const bagitem_t *bag_iterate(const bag_t *b);

/**
 * Returns the next item in the bag.
 *
 * @param it current item
 * @return next item, or NULL if no item left
 */
const bagitem_t *bag_next(const bagitem_t *it);

/**
 * Removes all elements from the bag.
 *
 * @param b bag object
 */
void bag_clear(bag_t *b);

/**
 * Perform a shallow copy content of src bag into dst bag.
 *
 * @note existing items in dst bag are not discarded.
 *
 * @param src source bag object
 * @param dst destination bag object
 */
void bag_copy(bag_t *dst, const bag_t *src);

#endif
