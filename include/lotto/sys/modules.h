/**
 * @file modules.h
 * @brief System wrapper declarations for modules.
 */
#ifndef LOTTO_MODULES_H
#define LOTTO_MODULES_H

#include <stdbool.h>
#include <stddef.h>

#define DRIVER_MODULE_PREFIX          "lotto-driver-"
#define DRIVER_MODULE_PREFIX_LEN      (sizeof(DRIVER_MODULE_PREFIX) - 1)
#define RUNTIME_MODULE_PREFIX         "lotto-runtime-"
#define RUNTIME_MODULE_PREFIX_LEN     (sizeof(RUNTIME_MODULE_PREFIX) - 1)
#define RUNTIME_DBG_MODULE_PREFIX     "lotto-runtime-dbg-"
#define RUNTIME_DBG_MODULE_PREFIX_LEN (sizeof(RUNTIME_DBG_MODULE_PREFIX) - 1)

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
    bool dbg;           //< runtime dbg variant
    bool shadowed;      //< whether it is shadowed by another module lib file
    bool disabled; //< disabled by explicit user module selection, or failed to
                   // load
} module_t;

typedef enum {
    LOTTO_MODULE_COMPONENT_DRIVER,
    LOTTO_MODULE_COMPONENT_RUNTIME,
} lotto_module_component_t;

typedef struct lotto_module_metadata_s {
    int slot;
    const char *name;
    lotto_module_component_t component;
    const char *type;
} lotto_module_metadata_t;

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

const lotto_module_metadata_t *lotto_module_metadata_all(size_t *count);

#endif
