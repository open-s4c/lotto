#ifndef LOTTO_MARSHABLE_H
#define LOTTO_MARSHABLE_H

/*******************************************************************************
 * @file marshable.h
 * @brief Generic marshable/unmarshable interface
 *
 * This generic interface allows for data structures to be serialized and
 * deserialized, for example, in a trace file.  To make a data structure become
 * marshable, add `marshable_t m` as the first field of the struct and implement
 * the interface accordingly.  When initializing the data structure, set `m` to
 * point to the desired implementation functions.
 ******************************************************************************/
#include <stddef.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>

/**
 * Marshable interface.
 *
 * Add a field `m` of this type to a marshable data structure.
 *
 * ```c
 * struct my_struct {
 *   marshable_t m;
 *   int some_field;
 *   char some_other_field[128];
 * };
 *
 * struct my_struct my_var;
 *
 * void init() {
 *    my_var.m = MARSHABLE_STATIC(sizeof(my_var));
 *    ...
 * }
 * ```
 */
typedef struct marshable {
    const void *(*unmarshal)(struct marshable *m, const void *buf);
    void *(*marshal)(const struct marshable *m, void *buf);
    size_t (*size)(const struct marshable *m);
    void (*print)(const struct marshable *m);

    size_t alloc_size;      //< minimal allocation size
    struct marshable *next; //< chained marshable
    char payload[0];        //< actual marshable payload
} marshable_t;

/**
 * Calculates the necessary size to marshal m.
 *
 * @param m marshable field of struct.
 * @return size of struct when marshaled.
 */
size_t marshable_size(const marshable_t *m);

/**
 * Marshals struct into buffer `buf`.
 *
 * Function assumes `buf` has at least `marshable_size(m)` bytes.
 *
 * @param m marshable field of struct.
 * @param buf buffer with enough space.
 * @return pointer relative to buf after marshaled m.
 */
void *marshable_marshal(const marshable_t *m, void *buf);

/**
 * Unmarshals struct from buffer `buf`
 *
 * @param m marshable field of struct.
 * @param buf buffer with marshaled struct.
 * @return pointer relative to buf after marshaled m.
 */
const void *marshable_unmarshal(marshable_t *m, const void *buf);

/**
 * Prints human readable representation of marshable struct.
 * @param m marshable field of struct.
 */
void marshable_print(marshable_t *m);

/**
 * Binds a child marshable struct to a parent marshable struct.
 *
 * The child struct bound to parent whenever the latter is marshaled,
 * unmarshaled, printed or its size is calculated.
 *
 * @param parent marshable field of parent struct.
 * @param child marshable field of child struct.
 */
void marshable_bind(marshable_t *parent, marshable_t *child);

/**
 * Copies marshable struct from `src` to `dst` via marshaling.
 *
 * @param dst target marshable struct.
 * @param src source marshable struct.
 */
void marshable_copy(marshable_t *dst, const marshable_t *src);

#define marshable_size_m(M)         marshable_size(&(M)->m)
#define marshable_marshal_m(M, B)   marshable_marshal(&(M)->m, B)
#define marshable_unmarshal_m(M, B) marshable_unmarshal(&(M)->m, B)
#define marshable_print_m(M)        marshable_print(&(M)->m)
#define marshable_copy_m(M, B)      marshable_copy(&(M)->m, &(B)->m)
#define marshable_bind_m(P, C)      marshable_bind(&(P)->m, &(C)->m)

size_t marshable_static_size(const marshable_t *m);
void *marshable_static_marshal(const marshable_t *m, void *buf);
const void *marshable_static_unmarshal(marshable_t *m, const void *buf);

#define MARSHABLE_STATIC_PRINTABLE(SIZE, PRINT)                                \
    ((marshable_t){                                                            \
        .marshal    = marshable_static_marshal,                                \
        .unmarshal  = marshable_static_unmarshal,                              \
        .size       = marshable_static_size,                                   \
        .print      = (PRINT),                                                 \
        .alloc_size = (SIZE),                                                  \
        .next       = NULL,                                                    \
    })

#define MARSHABLE_STATIC(SIZE) MARSHABLE_STATIC_PRINTABLE(SIZE, NULL)

#endif /* LOTTO_MARSHABLE_H */
