/*
 */

#include <stdlib.h>

#include <lotto/base/address_bdd.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>

#define ADDRESS_BDD_FALSE (uintptr_t) NULL
#define ADDRESS_BDD_TRUE  ~ADDRESS_BDD_FALSE

struct address_bdd_s {
    address_bdd_node_t *root;
    uint8_t max_depth;
    uintptr_t size_unit;
};

struct address_bdd_node_s {
    address_bdd_node_t *parent;
    address_bdd_node_t *one;
    address_bdd_node_t *zero;
};

struct address_bdd_iterator_s {
    address_bdd_node_t *node;
    address_bdd_node_t *parent_node;
    uintptr_t current_bit;
    uintptr_t offset;
    uintptr_t base;
};

static bool _node_is_terminal(address_bdd_node_t *node);
static void _free_node(address_bdd_node_t *node);
static uintptr_t _ceil_address(void *addr, uintptr_t unit);
static uintptr_t _floor_address(void *addr, uintptr_t unit);
static void _address_bdd_set_range(address_bdd_node_t *parent_node,
                                   address_bdd_node_t **node_ptr,
                                   uintptr_t addr_start, uintptr_t addr_end,
                                   uintptr_t current_start,
                                   uintptr_t current_end, uintptr_t current_bit,
                                   uintptr_t value);
static bool _address_bdd_check_range(address_bdd_node_t **node_ptr,
                                     uintptr_t addr_start, uintptr_t addr_end,
                                     uintptr_t current_start,
                                     uintptr_t current_end,
                                     uintptr_t current_bit, uintptr_t value);
static void _node_split(address_bdd_node_t *parent_node,
                        address_bdd_node_t **node_ptr);
static bool _address_bdd_range_print(address_bdd_node_t *node_ptr,
                                     uintptr_t current_start,
                                     uintptr_t current_end,
                                     uintptr_t current_bit, uintptr_t value,
                                     bool interval_open);
static bool _value_is_terminal(uintptr_t value);
static address_bdd_t *_address_bdd_new(uint8_t max_depth, uintptr_t init_value);
static void _address_bdd_set(address_bdd_t *bdd, void *addr, size_t size,
                             uintptr_t value);
static void _address_bdd_print(address_bdd_t *bdd, uintptr_t value);
static uintptr_t _leading_one();

void
address_bdd_set(address_bdd_t *bdd, void *addr, size_t size)
{
    _address_bdd_set(bdd, addr, size, ADDRESS_BDD_TRUE);
}

void
address_bdd_unset(address_bdd_t *bdd, void *addr, size_t size)
{
    _address_bdd_set(bdd, addr, size, ADDRESS_BDD_FALSE);
}

static void
_address_bdd_set(address_bdd_t *bdd, void *addr, size_t size, uintptr_t value)
{
    if (size == 0) {
        return;
    }
    ASSERT(_value_is_terminal(value));
    address_bdd_node_t **const node_ptr = &bdd->root;
    const uintptr_t addr_start          = _floor_address(addr, bdd->size_unit);
    const uintptr_t addr_end = _ceil_address(addr + size - 1, bdd->size_unit);
    const uintptr_t current_start = 0;
    const uintptr_t current_end   = ~(uintptr_t)0;
    const uintptr_t current_bit   = _leading_one();

    _address_bdd_set_range(NULL, node_ptr, addr_start, addr_end, current_start,
                           current_end, current_bit, value);
}

static void
_address_bdd_set_range(address_bdd_node_t *parent_node,
                       address_bdd_node_t **node_ptr, uintptr_t addr_start,
                       uintptr_t addr_end, uintptr_t current_start,
                       uintptr_t current_end, uintptr_t current_bit,
                       uintptr_t value)
{
    if (addr_start > current_end || addr_end < current_start ||
        *node_ptr == (void *)value) {
        return;
    }
    if (addr_start <= current_start && addr_end >= current_end) {
        _free_node(*node_ptr);
        *node_ptr = (void *)value;
        return;
    }
    if (_node_is_terminal(*node_ptr)) {
        _node_split(parent_node, node_ptr);
    }
    _address_bdd_set_range(*node_ptr, &(*node_ptr)->zero, addr_start, addr_end,
                           current_start, current_end ^ current_bit,
                           current_bit >> 1, value);
    _address_bdd_set_range(*node_ptr, &(*node_ptr)->one, addr_start, addr_end,
                           current_start ^ current_bit, current_end,
                           current_bit >> 1, value);
    if ((*node_ptr)->zero == (*node_ptr)->one) {
        address_bdd_node_t *value = (*node_ptr)->zero;
        _free_node(*node_ptr);
        *node_ptr = value;
    }
}

static uintptr_t
_leading_one()
{
    return ((uintptr_t)1) << ((sizeof(uintptr_t) << 3) - 1);
}

static void
_node_split(address_bdd_node_t *parent_node, address_bdd_node_t **node_ptr)
{
    address_bdd_node_t *value = *node_ptr;
    *node_ptr                 = sys_malloc(sizeof(address_bdd_node_t));
    (*node_ptr)->zero         = value;
    (*node_ptr)->one          = value;
    (*node_ptr)->parent       = parent_node;
}

bool
address_bdd_get_and(address_bdd_t *bdd, void *addr, size_t size)
{
    address_bdd_node_t **const root_node_ptr = &bdd->root;
    const uintptr_t addr_start               = (uintptr_t)addr;
    const uintptr_t addr_end                 = (uintptr_t)addr + size - 1;
    const uintptr_t current_start            = 0;
    const uintptr_t current_end              = ~(uintptr_t)0;
    const uintptr_t current_bit              = _leading_one();
    return _address_bdd_check_range(root_node_ptr, addr_start, addr_end,
                                    current_start, current_end, current_bit,
                                    ADDRESS_BDD_TRUE);
}
bool
address_bdd_get_or(address_bdd_t *bdd, void *addr, size_t size)
{
    address_bdd_node_t **const root_node_ptr = &bdd->root;
    const uintptr_t addr_start               = (uintptr_t)addr;
    const uintptr_t addr_end                 = (uintptr_t)addr + size - 1;
    const uintptr_t current_start            = 0;
    const uintptr_t current_end              = ~(uintptr_t)0;
    const uintptr_t current_bit              = _leading_one();
    return !_address_bdd_check_range(root_node_ptr, addr_start, addr_end,
                                     current_start, current_end, current_bit,
                                     ADDRESS_BDD_FALSE);
}

static bool
_address_bdd_check_range(address_bdd_node_t **node_ptr, uintptr_t addr_start,
                         uintptr_t addr_end, uintptr_t current_start,
                         uintptr_t current_end, uintptr_t current_bit,
                         uintptr_t value)
{
    if (addr_start > current_end || addr_end < current_start ||
        *node_ptr == (void *)value) {
        return true;
    }
    if (_node_is_terminal(*node_ptr)) {
        return *node_ptr == (void *)value;
    }
    return _address_bdd_check_range(&(*node_ptr)->zero, addr_start, addr_end,
                                    current_start, current_end ^ current_bit,
                                    current_bit >> 1, value) &&
           _address_bdd_check_range(&(*node_ptr)->one, addr_start, addr_end,
                                    current_start ^ current_bit, current_end,
                                    current_bit >> 1, value);
}

address_bdd_t *
address_bdd_new(uint8_t max_depth)
{
    return _address_bdd_new(max_depth, ADDRESS_BDD_FALSE);
}

static address_bdd_t *
_address_bdd_new(uint8_t max_depth, uintptr_t init_value)
{
    ASSERT(_value_is_terminal(init_value));
    ASSERT(max_depth <= 64);
    address_bdd_t *result = sys_malloc(sizeof(address_bdd_t));
    result->max_depth     = max_depth;
    result->root          = (void *)init_value;
    result->size_unit     = ((uintptr_t)1)
                        << ((sizeof(uintptr_t) << 3) - max_depth);
    return result;
}

void
address_bdd_free(address_bdd_t *bdd)
{
    _free_node(bdd->root);
    sys_free(bdd);
}

static uintptr_t
_ceil_address(void *addr, uintptr_t unit)
{
    return _floor_address(addr, unit) + unit - 1;
}

static uintptr_t
_floor_address(void *addr, uintptr_t unit)
{
    return ((uintptr_t)addr) & ~(unit - 1);
}

static void
_free_node(address_bdd_node_t *node)
{
    if (_node_is_terminal(node)) {
        return;
    }
    _free_node(node->one);
    _free_node(node->zero);
    sys_free(node);
}

static bool
_node_is_terminal(address_bdd_node_t *node)
{
    return _value_is_terminal((uintptr_t)node);
}

static bool
_value_is_terminal(uintptr_t value)
{
    return value == ADDRESS_BDD_FALSE || value == ADDRESS_BDD_TRUE;
}

void
address_bdd_print(address_bdd_t *bdd)
{
    _address_bdd_print(bdd, ADDRESS_BDD_TRUE);
}

static void
_address_bdd_print(address_bdd_t *bdd, uintptr_t value)
{
    address_bdd_node_t *const node_ptr = bdd->root;
    const uintptr_t current_start      = 0;
    const uintptr_t current_end        = ~(uintptr_t)0;
    const uintptr_t current_bit        = _leading_one();
    if (_address_bdd_range_print(node_ptr, current_start, current_end,
                                 current_bit, value, false)) {
        logger_infof("0x%016lx\n", ~(uintptr_t)0);
    }
}

static bool
_address_bdd_range_print(address_bdd_node_t *node_ptr, uintptr_t current_start,
                         uintptr_t current_end, uintptr_t current_bit,
                         uintptr_t value, bool interval_open)
{
    if (_node_is_terminal(node_ptr)) {
        if (node_ptr != (void *)value) {
            if (interval_open) {
                logger_infof("0x%016lx\n", current_start - 1);
            }
            return false;
        }
        if (!interval_open) {
            logger_infof("0x%016lx-", current_start);
        }
        return true;
    }
    return _address_bdd_range_print(
        node_ptr->one, current_start ^ current_bit, current_end,
        current_bit >> 1, value,
        _address_bdd_range_print(node_ptr->zero, current_start,
                                 current_end ^ current_bit, current_bit >> 1,
                                 value, interval_open));
}

uintptr_t
_get_masked(uintptr_t addr, uint64_t mask, address_bdd_node_t *current_node,
            uintptr_t current_bit)
{
    if ((uintptr_t)current_node == ADDRESS_BDD_TRUE) {
        return addr;
    }
    if ((uintptr_t)current_node == ADDRESS_BDD_FALSE) {
        return ADDRESS_BDD_FALSE;
    }
    if (current_bit & mask) {
        address_bdd_node_t *next_node =
            current_bit & addr ? current_node->one : current_node->zero;
        return _get_masked(addr, mask, next_node, current_bit >> 1);
    } else {
        uintptr_t result =
            _get_masked(addr, mask, current_node->one, current_bit >> 1);
        if (result != ADDRESS_BDD_FALSE) {
            return result | current_bit;
        }
        return _get_masked(addr, mask, current_node->zero, current_bit >> 1) &
               ~current_bit;
    }
}

void *
address_bdd_get_masked(address_bdd_t *bdd, void *addr, uint64_t mask)
{
    return (void *)_get_masked((uintptr_t)addr, mask, bdd->root,
                               _leading_one());
}

static bool
_find_true_sub_node(address_bdd_iterator_t *iterator,
                    address_bdd_node_t *current_node,
                    address_bdd_node_t *parent_node, uintptr_t current_bit,
                    uintptr_t base)
{
    if ((uintptr_t)current_node == ADDRESS_BDD_TRUE) {
        iterator->node        = current_node;
        iterator->parent_node = parent_node;
        iterator->base        = base;
        iterator->offset      = base == 0;
        iterator->current_bit = current_bit;
        return true;
    }
    if ((uintptr_t)current_node == ADDRESS_BDD_FALSE) {
        return false;
    }
    current_bit = current_bit ? current_bit >> 1 : _leading_one();
    return _find_true_sub_node(iterator, current_node->zero, current_node,
                               current_bit, base) ||
           _find_true_sub_node(iterator, current_node->one, current_node,
                               current_bit, base | current_bit);
}

address_bdd_iterator_t *
address_bdd_iterator(address_bdd_t *bdd)
{
    address_bdd_iterator_t *iterator =
        sys_malloc(sizeof(address_bdd_iterator_t));
    *iterator = (address_bdd_iterator_t){.node = bdd->root};
    if (!_find_true_sub_node(iterator, iterator->node, iterator->parent_node,
                             iterator->current_bit, iterator->base)) {
        iterator->offset = ADDRESS_BDD_TRUE;
    }
    return iterator;
}

static bool
_find_next_true_node(address_bdd_iterator_t *iterator)
{
    if (!iterator->parent_node) {
        return false;
    }
    if (iterator->base & iterator->current_bit) {
        iterator->base ^= iterator->current_bit;
        iterator->current_bit <<= 1;
        iterator->node        = iterator->parent_node;
        iterator->parent_node = iterator->parent_node->parent;
        return _find_next_true_node(iterator);
    }
    iterator->base ^= iterator->current_bit;
    iterator->node = iterator->parent_node->one;
    return _find_true_sub_node(iterator, iterator->node, iterator->parent_node,
                               iterator->current_bit, iterator->base) ||
           _find_next_true_node(iterator);
}

uintptr_t
address_bdd_iterator_next(address_bdd_iterator_t *iterator)
{
    if (iterator->base + iterator->offset == ADDRESS_BDD_TRUE ||
        (iterator->offset == iterator->current_bit &&
         !_find_next_true_node(iterator))) {
        return ADDRESS_BDD_FALSE;
    }
    ASSERT((uintptr_t)iterator->node == ADDRESS_BDD_TRUE);
    return iterator->base + iterator->offset++;
}

void
address_bdd_iterator_free(address_bdd_iterator_t *iterator)
{
    sys_free(iterator);
}

size_t
address_bdd_size(address_bdd_t *bdd)
{
    address_bdd_iterator_t *iterator = address_bdd_iterator(bdd);
    size_t result                    = 0;
    while (address_bdd_iterator_next(iterator)) {
        result++;
    }
    return result;
}
