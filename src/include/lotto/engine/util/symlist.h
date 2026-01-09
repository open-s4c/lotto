/*
 */
#ifndef SYMLIST_H
#define SYMLIST_H

#include <stddef.h>
#include <stdint.h>

typedef struct sym_node {
    uint64_t addr;
    const char *sym_name;
    struct sym_node *next;
} sym_node_t;

typedef struct sym_node_list {
    uint64_t mask;
    const char *f_name;
    sym_node_t *sym_head;
    struct sym_node_list *next;
} sym_node_list_t;

extern sym_node_list_t *sym_node_list_head;

#endif
