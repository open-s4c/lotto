#ifndef LOTTO_MODULES_H
#define LOTTO_MODULES_H

#include <stdbool.h>

typedef enum {
    MODULE_KIND_NONE    = 0,
    MODULE_KIND_CLI     = 1,
    MODULE_KIND_ENGINE  = 1 << 1,
    MODULE_KIND_RUNTIME = 1 << 2,
    MODULE_KIND_MEMMGR  = 1 << 3,
} module_kind_t;

#define MODULE_ALL (~0U)

typedef struct module_s {
    char *name; //< module name, e.g. 'priority'
    char *path; //< module path, e.g. '/PATH/TO/liblotto_priority_engine.so'
    module_kind_t kind; //< module kind
    bool shadowed;      //< whether it is shadowed by another module lib file
    bool disabled; //< disabled by explicit user module selection, or failed to
                   // load
} module_t;

typedef int (*module_foreach_f)(module_t *, void *);

/** Scan the module dirs and populate the module registry. */
void lotto_module_scan(const char *build_dir, const char *install_dir,
                       const char *module_dir);

/** Disables all but the comma seperated list of modules. Does nothing if NULL
 * is passed. */
int lotto_module_enable_only(const char *module_list);

/** Clear the module registry. */
void lotto_module_clear();

/** Print the module registry to stdout. */
void lotto_module_print();
const char *lotto_module_kind_str(module_kind_t kind);

/** Do something on each module.
 * If some f returns a non-zero value, the function returns immediately that
 * value. If all f's return zero, the function returns zero.
 */
int lotto_module_foreach(module_foreach_f f, void *arg);
int lotto_module_foreach_all(module_foreach_f f, void *arg);

/** Similar to lotto_module_foreach but in the reverse order. */
int lotto_module_foreach_reverse(module_foreach_f f, void *arg);

#endif
