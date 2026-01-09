/*
 */
#ifndef LOTTO_PLUGIN_H
#define LOTTO_PLUGIN_H

#include <stdbool.h>

typedef enum {
    PLUGIN_KIND_NONE    = 0,
    PLUGIN_KIND_CLI     = 1,
    PLUGIN_KIND_ENGINE  = 1 << 1,
    PLUGIN_KIND_RUNTIME = 1 << 2,
    PLUGIN_KIND_MEMMGR  = 1 << 3,
} plugin_kind_t;

#define PLUGIN_ALL (~0U)

typedef struct plugin_s {
    char *name; //< plugin name, e.g. 'priority'
    char *path; //< plugin path, e.g. '/PATH/TO/liblotto_priority_engine.so'
    plugin_kind_t kind; //< plugin kind
    bool shadowed;      //< whether it is shadowed by another plugin lib file
    bool disabled; //< disabled by explicit user plugin selection, or failed to
                   // load
} plugin_t;

typedef int (*plugin_foreach_f)(plugin_t *, void *);

/** Scan the plugin dirs and populate the plugin registry. */
void lotto_plugin_scan(const char *build_dir, const char *install_dir,
                       const char *plugin_dir);

/** Disables all but the comma seperated list of plugins. Does nothing if NULL
 * is passed. */
int lotto_plugin_enable_only(const char *plugin_list);

/** Clear the plugin registry. */
void lotto_plugin_clear();

/** Print the plugin registry to stdout. */
void lotto_plugin_print();

/** Do something on each plugin.
 * If some f returns a non-zero value, the function returns immediately that
 * value. If all f's return zero, the function returns zero.
 */
int lotto_plugin_foreach(plugin_foreach_f f, void *arg);

/** Similar to lotto_plugin_foreach but in the reverse order. */
int lotto_plugin_foreach_reverse(plugin_foreach_f f, void *arg);

#endif
