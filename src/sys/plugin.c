/*
 */
#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/memory.h>
#include <lotto/sys/plugin.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/string.h>
#include <lotto/util/iterator.h>
#include <lotto/util/strings.h>

#define MAX_PLUGINS            100
#define MAX_PLUGIN_NAME_LENGTH 1023
#define PLUGIN_PREFIX          "liblotto_"
#define PLUGIN_PREFIX_LEN      (sizeof(PLUGIN_PREFIX) - 1)

#define ENGINE_PLUGIN_SUFFIX     "engine.so"
#define ENGINE_PLUGIN_SUFFIX_LEN (sizeof(ENGINE_PLUGIN_SUFFIX) - 1)

#define CLI_PLUGIN_SUFFIX     "cli.so"
#define CLI_PLUGIN_SUFFIX_LEN (sizeof(CLU_PLUGIN_SUFFIX) - 1)

#define RUNTIME_PLUGIN_SUFFIX     "runtime.so"
#define RUNTIME_PLUGIN_SUFFIX_LEN (sizeof(RUNTIME_PLUGIN_SUFFIX) - 1)

static plugin_t _plugins[MAX_PLUGINS];
static size_t _next = 0;

static char *_plugin_name(const char *filename);
static int _scandir(const char *dir);
static int _compar(const void *, const void *);
static bool _ends_with(const char *, const char *);

typedef struct name_dist_s {
    const char *name;
    uint64_t dist;
} name_dist_t;

static void map_dist(plugin_t *plugin, iter_args_t *map_arg);
static void reduce_dist(iter_args_t *map_returns, uint64_t size,
                        iter_args_t *reduce_arg);

#define ITERATE_PLUGINS(MAP, MAP_ARG, TYPE_MAT_RET, REDUCE, REDUCE_ARG,        \
                        REDUCE_RET)                                            \
    ITERATE_ARRAY(_plugins, plugin_t, _next, MAP, MAP_ARG, TYPE_MAT_RET,       \
                  REDUCE, REDUCE_ARG, REDUCE_RET)

#define ITERATE_PLUGINS_CLOSEST_NAME(MAP_ARG, REDUCE_RET)                      \
    ITERATE_PLUGINS(map_dist, MAP_ARG, name_dist_t, reduce_dist, NULL,         \
                    REDUCE_RET)

// closest plugin name map fun
static void
map_dist(plugin_t *plugin, iter_args_t *map_arg)
{
    name_dist_t *arg = (name_dist_t *)map_arg->arg;
    name_dist_t *ret = (name_dist_t *)map_arg->ret;
    ret->dist        = levenshtein(plugin->name, arg->name);
    ret->name        = plugin->name;
}

static void
reduce_dist(iter_args_t *map_returns, uint64_t size, iter_args_t *reduce_arg)
{
    // name_dist_t *arg = (name_dist_t *)reduce_arg->arg;
    name_dist_t *ret = (name_dist_t *)reduce_arg->ret;
    ret->dist        = UINT64_MAX;
    for (uint64_t i = 0; i < size; i++) {
        name_dist_t *map_ret = (name_dist_t *)map_returns[i].ret;
        if (map_ret->dist < ret->dist) {
            ret->dist = map_ret->dist;
            ret->name = map_ret->name;
        }
    }
}

void
lotto_plugin_scan(const char *build_dir, const char *install_dir,
                  const char *plugin_dir)
{
    lotto_plugin_clear();
    if (install_dir)
        _scandir(install_dir);
    if (build_dir)
        _scandir(build_dir);
    if (plugin_dir && plugin_dir[0] != '\0') {
        log_infof("Using external plugins directory: %s\n", plugin_dir);
        _scandir(plugin_dir);
    }
    qsort(_plugins, _next, sizeof(plugin_t), _compar);
}

static int
_plugin_disable(plugin_t *plugin, void *arg)
{
    (void)arg;
    plugin->disabled = true;
    return 0;
}

static int
_plugin_enable_by_name(const char *plugin_name)
{
    int not_found = 1;
    for (size_t i = 0; i < _next; ++i) {
        plugin_t *plugin = &_plugins[i];
        if (plugin->shadowed)
            continue;
        if (!(plugin->disabled))
            continue;
        if (strcmp(plugin->name, plugin_name) == 0) {
            plugin->disabled = false;
            not_found        = 0;
        }
    }
    return not_found;
}

static const char *
_plugin_find_name_closest(const char *plugin_name)
{
    name_dist_t map_dist_arg;
    name_dist_t reduce_dist_ret = {.name = ""};
    map_dist_arg.name           = plugin_name;

    ITERATE_PLUGINS_CLOSEST_NAME(&map_dist_arg, &reduce_dist_ret)

    if (reduce_dist_ret.dist <= 4) {
        return reduce_dist_ret.name;
    }
    return NULL;
}

// relies on plugins being sorted by name
static void
_plugin_print_names_unique()
{
    uint32_t num_plugins_unique     = 0;
    uint32_t cur_num_plugins_unique = 0;
    // count number of unique plugin names
    for (size_t i = 0; i < _next; ++i) {
        if (i > 0 && strcmp(_plugins[i].name, _plugins[i - 1].name) == 0)
            continue;
        num_plugins_unique++;
    }

    log_infof("Found %u plugin names: ", num_plugins_unique);
    for (size_t i = 0; i < _next; ++i) {
        if (i > 0 && strcmp(_plugins[i].name, _plugins[i - 1].name) == 0)
            continue;
        log_infof("%s", _plugins[i].name);
        cur_num_plugins_unique++;
        if (cur_num_plugins_unique < num_plugins_unique)
            log_infof(",");
        else
            log_infof("\n");
    }
}

int
lotto_plugin_enable_only(const char *plugin_list)
{
    char cur_plugin_name[MAX_PLUGIN_NAME_LENGTH + 1];
    const char *cur_plugin_ptr = plugin_list;
    const char *next_sep_ptr   = NULL;
    uint64_t cur_plugin_len    = 0;

    // just to be explicit, but actually unnecessary as
    // while(NULL != cur_plugin_ptr) captures it also
    if (plugin_list == NULL)
        return 0;

    lotto_plugin_foreach(_plugin_disable, NULL);

    // go over plugin_list
    while (NULL != cur_plugin_ptr) {
        next_sep_ptr = strstr(cur_plugin_ptr, ",");

        if (NULL != next_sep_ptr)
            cur_plugin_len = next_sep_ptr - cur_plugin_ptr;
        else
            cur_plugin_len = sys_strlen(cur_plugin_ptr);

        ASSERT(cur_plugin_len > 0 && cur_plugin_len <= MAX_PLUGIN_NAME_LENGTH);
        strncpy(cur_plugin_name, cur_plugin_ptr, cur_plugin_len);
        cur_plugin_name[cur_plugin_len] = '\0';

        // do something with current plugin name from plugin_list

        bool plugin_found               = false;
        const char *closest_plugin_name = NULL;
        if (0 == _plugin_enable_by_name(cur_plugin_name))
            plugin_found = true;
        if (!plugin_found) {
            closest_plugin_name = _plugin_find_name_closest(cur_plugin_name);
            log_infof("Could not find plugin %s\n", cur_plugin_name);
            if (NULL != closest_plugin_name) {
                log_infof("Did you mean: %s ?\n", closest_plugin_name);
            }
            _plugin_print_names_unique();
            return 1;
        }

        // go to next plugin in list
        if (NULL != next_sep_ptr)
            cur_plugin_ptr = next_sep_ptr + 1;
        else
            cur_plugin_ptr = NULL;
    }
    return 0;
}

void
lotto_plugin_clear()
{
    for (size_t i = 0; i < _next; ++i) {
        sys_free(_plugins[i].name);
        sys_free(_plugins[i].path);
        _plugins[i].name = _plugins[i].path = NULL;
    }
    _next = 0;
}

int
lotto_plugin_foreach(plugin_foreach_f f, void *arg)
{
    for (size_t i = 0; i < _next; ++i) {
        plugin_t *plugin = &_plugins[i];
        if (plugin->shadowed)
            continue;
        if (plugin->disabled)
            continue;
        int val = f(plugin, arg);
        if (val != 0)
            return val;
    }
    return 0;
}

int
lotto_plugin_foreach_reverse(plugin_foreach_f f, void *arg)
{
    for (size_t i = _next; i; --i) {
        plugin_t *plugin = &_plugins[i - 1];
        if (plugin->shadowed)
            continue;
        if (plugin->disabled)
            continue;
        int val = f(plugin, arg);
        if (val != 0)
            return val;
    }
    return 0;
}

static const char *
_plugin_kind_str(plugin_kind_t kind)
{
    switch (kind) {
        case PLUGIN_KIND_NONE:
            return "NONE";
        case PLUGIN_KIND_CLI:
            return "CLI";
        case PLUGIN_KIND_ENGINE:
            return "ENGINE";
        case PLUGIN_KIND_RUNTIME:
            return "RUNTIME";
        default:
            log_fatalf("Unknown plugin kind value %u\n", kind);
            return "";
    }
}

void
lotto_plugin_print()
{
    log_infof("Found %lu plugins\n", _next);
    for (size_t i = 0; i < _next; ++i) {
        log_infof(
            "Plugin '%s' %s found at '%s', shadowed = %s, disabled = %s\n",
            _plugins[i].name, _plugin_kind_str(_plugins[i].kind),
            _plugins[i].path, _plugins[i].shadowed ? "yes" : "no",
            _plugins[i].disabled ? "yes" : "no");
    }
}

static char *
_plugin_name(const char *filename)
{
    filename += PLUGIN_PREFIX_LEN;
    size_t len = strrchr(filename, '_') - filename;
    return sys_strndup(filename, len);
}

static bool
_ends_with(const char *a, const char *b)
{
    size_t al = sys_strlen(a), bl = sys_strlen(b);
    if (al < bl) {
        return false;
    }
    return sys_strncmp(a + al - bl, b, bl) == 0;
}

static int
_scandir(const char *scan_dir)
{
    if (access(scan_dir, F_OK) != 0)
        return 0;
    DIR *dir = opendir(scan_dir);
    if (!dir)
        return -1;
    char path[PATH_MAX];
    for (struct dirent *entry; (entry = readdir(dir));) {
        if (sys_strncmp(entry->d_name, PLUGIN_PREFIX, PLUGIN_PREFIX_LEN) != 0) {
            continue;
        }
        char *name         = _plugin_name(entry->d_name);
        plugin_kind_t kind = PLUGIN_KIND_NONE;
        if (_ends_with(entry->d_name, CLI_PLUGIN_SUFFIX)) {
            kind |= PLUGIN_KIND_CLI;
        }
        if (_ends_with(entry->d_name, ENGINE_PLUGIN_SUFFIX)) {
            kind |= PLUGIN_KIND_ENGINE;
        }
        if (_ends_with(entry->d_name, RUNTIME_PLUGIN_SUFFIX)) {
            kind |= PLUGIN_KIND_RUNTIME;
        }
        if (strstr(entry->d_name, "mempool_") != NULL ||
            strstr(entry->d_name, "uaf_") != NULL ||
            strstr(entry->d_name, "leakcheck_") != NULL) {
            kind |= PLUGIN_KIND_MEMMGR;
        }
        if (kind == PLUGIN_KIND_NONE) {
            log_fatalf("unknown kind of plugin %s\n", entry->d_name);
        }
        for (size_t i = 0; i < _next; ++i) {
            if (!strcmp(name, _plugins[i].name) && kind == _plugins[i].kind) {
                _plugins[i].shadowed = true;
                _plugins[i].disabled = true;
            }
        }
        sys_snprintf(path, PATH_MAX, "%s/%s", scan_dir, entry->d_name);
        plugin_t plugin = {.name     = name,
                           .path     = sys_strdup(path),
                           .kind     = kind,
                           .shadowed = false,
                           .disabled = false};
        log_debugf("Found new plugin '%s'\n", plugin.path);
        _plugins[_next++] = plugin;
    }
    closedir(dir);
    return 0;
}

static int
_compar(const void *_p, const void *_q)
{
    const plugin_t *p = (const plugin_t *)_p;
    const plugin_t *q = (const plugin_t *)_q;
    int val;
    if ((val = strcmp(p->name, q->name)))
        return val;
    if ((val = (int)p->kind - (int)q->kind)) {
        return val;
    }
    return 0;
}
