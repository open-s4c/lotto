#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/memory.h>
#include <lotto/sys/modules.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/string.h>
#include <lotto/util/iterator.h>
#include <lotto/util/strings.h>

#define MAX_MODULES            100
#define MAX_MODULE_NAME_LENGTH 1023
#define SO_SUFFIX              ".so"
#define SO_SUFFIX_LEN          (sizeof(SO_SUFFIX) - 1)

#define DRIVER_MODULE_PREFIX          "lotto-driver-"
#define DRIVER_MODULE_PREFIX_LEN      (sizeof(DRIVER_MODULE_PREFIX) - 1)
#define RUNTIME_MODULE_PREFIX         "lotto-runtime-"
#define RUNTIME_MODULE_PREFIX_LEN     (sizeof(RUNTIME_MODULE_PREFIX) - 1)
#define RUNTIME_VERBOSE_MODULE_PREFIX "lotto-runtime-verbose-"
#define RUNTIME_VERBOSE_MODULE_PREFIX_LEN                                      \
    (sizeof(RUNTIME_VERBOSE_MODULE_PREFIX) - 1)

#define STARTS_WITH(s, LITERAL_NAME)                                           \
    (sys_strncmp((s), LITERAL_NAME, LITERAL_NAME##_LEN) == 0)

static module_t _modules[MAX_MODULES];
static size_t _next = 0;

static char *_module_name(const char *filename);
static int _scandir(const char *dir);
static int _compar(const void *, const void *);
static bool _ends_with(const char *, const char *);

typedef struct name_dist_s {
    const char *name;
    uint64_t dist;
} name_dist_t;

static void map_dist(module_t *module, iter_args_t *map_arg);
static void reduce_dist(iter_args_t *map_returns, uint64_t size,
                        iter_args_t *reduce_arg);

#define ITERATE_MODULES(MAP, MAP_ARG, TYPE_MAT_RET, REDUCE, REDUCE_ARG,        \
                        REDUCE_RET)                                            \
    ITERATE_ARRAY(_modules, module_t, _next, MAP, MAP_ARG, TYPE_MAT_RET,       \
                  REDUCE, REDUCE_ARG, REDUCE_RET)

#define ITERATE_MODULES_CLOSEST_NAME(MAP_ARG, REDUCE_RET)                      \
    ITERATE_MODULES(map_dist, MAP_ARG, name_dist_t, reduce_dist, NULL,         \
                    REDUCE_RET)

// closest module name map fun
static void
map_dist(module_t *module, iter_args_t *map_arg)
{
    name_dist_t *arg = (name_dist_t *)map_arg->arg;
    name_dist_t *ret = (name_dist_t *)map_arg->ret;
    ret->dist        = levenshtein(module->name, arg->name);
    ret->name        = module->name;
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
lotto_module_scan(const char *build_dir, const char *install_dir,
                  const char *module_dir)
{
    lotto_module_clear();
    if (install_dir)
        _scandir(install_dir);
    if (build_dir)
        _scandir(build_dir);
    if (module_dir && module_dir[0] != '\0') {
        logger_infof("Using external modules directory: %s\n", module_dir);
        _scandir(module_dir);
    }
    qsort(_modules, _next, sizeof(module_t), _compar);
}

static int
_disable_module(module_t *module, void *arg)
{
    (void)arg;
    module->disabled = true;
    return 0;
}

static int
_module_enable_by_name(const char *module_name)
{
    int not_found = 1;
    for (size_t i = 0; i < _next; ++i) {
        module_t *module = &_modules[i];
        if (module->shadowed)
            continue;
        if (!(module->disabled))
            continue;
        if (strcmp(module->name, module_name) == 0) {
            module->disabled = false;
            not_found        = 0;
        }
    }
    return not_found;
}

static const char *
_module_find_name_closest(const char *module_name)
{
    name_dist_t map_dist_arg;
    name_dist_t reduce_dist_ret = {.name = ""};
    map_dist_arg.name           = module_name;

    ITERATE_MODULES_CLOSEST_NAME(&map_dist_arg, &reduce_dist_ret)

    if (reduce_dist_ret.dist <= 4) {
        return reduce_dist_ret.name;
    }
    return NULL;
}

// relies on modules being sorted by name
static void
_module_print_names_unique()
{
    uint32_t num_modules_unique     = 0;
    uint32_t cur_num_modules_unique = 0;
    // count number of unique module names
    for (size_t i = 0; i < _next; ++i) {
        if (i > 0 && strcmp(_modules[i].name, _modules[i - 1].name) == 0)
            continue;
        num_modules_unique++;
    }

    logger_infof("Found %u module names: ", num_modules_unique);
    for (size_t i = 0; i < _next; ++i) {
        if (i > 0 && strcmp(_modules[i].name, _modules[i - 1].name) == 0)
            continue;
        logger_infof("%s", _modules[i].name);
        cur_num_modules_unique++;
        if (cur_num_modules_unique < num_modules_unique)
            logger_infof(",");
        else
            logger_infof("\n");
    }
}

int
lotto_module_enable_only(const char *module_list)
{
    char cur_module_name[MAX_MODULE_NAME_LENGTH + 1];
    const char *cur_module_ptr = module_list;
    const char *next_sep_ptr   = NULL;
    uint64_t cur_module_len    = 0;

    // just to be explicit, but actually unnecessary as
    // while(NULL != cur_module_ptr) captures it also
    if (module_list == NULL)
        return 0;

    lotto_module_foreach(_disable_module, NULL);

    // go over module_list
    while (NULL != cur_module_ptr) {
        next_sep_ptr = strstr(cur_module_ptr, ",");

        if (NULL != next_sep_ptr)
            cur_module_len = next_sep_ptr - cur_module_ptr;
        else
            cur_module_len = sys_strlen(cur_module_ptr);

        ASSERT(cur_module_len > 0 && cur_module_len <= MAX_MODULE_NAME_LENGTH);
        strncpy(cur_module_name, cur_module_ptr, cur_module_len);
        cur_module_name[cur_module_len] = '\0';

        // do something with current module name from module_list

        bool module_found               = false;
        const char *closest_module_name = NULL;
        if (0 == _module_enable_by_name(cur_module_name))
            module_found = true;
        if (!module_found) {
            closest_module_name = _module_find_name_closest(cur_module_name);
            logger_infof("Could not find module %s\n", cur_module_name);
            if (NULL != closest_module_name) {
                logger_infof("Did you mean: %s ?\n", closest_module_name);
            }
            _module_print_names_unique();
            return 1;
        }

        // go to next module in list
        if (NULL != next_sep_ptr)
            cur_module_ptr = next_sep_ptr + 1;
        else
            cur_module_ptr = NULL;
    }
    return 0;
}

void
lotto_module_clear()
{
    for (size_t i = 0; i < _next; ++i) {
        sys_free(_modules[i].name);
        sys_free(_modules[i].path);
        _modules[i].name = _modules[i].path = NULL;
    }
    _next = 0;
}

int
lotto_module_foreach(module_foreach_f f, void *arg)
{
    for (size_t i = 0; i < _next; ++i) {
        module_t *module = &_modules[i];
        if (module->shadowed)
            continue;
        if (module->disabled)
            continue;
        int val = f(module, arg);
        if (val != 0)
            return val;
    }
    return 0;
}

int
lotto_module_foreach_all(module_foreach_f f, void *arg)
{
    for (size_t i = 0; i < _next; ++i) {
        module_t *module = &_modules[i];
        int val          = f(module, arg);
        if (val != 0)
            return val;
    }
    return 0;
}

int
lotto_module_foreach_reverse(module_foreach_f f, void *arg)
{
    for (size_t i = _next; i; --i) {
        module_t *module = &_modules[i - 1];
        if (module->shadowed)
            continue;
        if (module->disabled)
            continue;
        int val = f(module, arg);
        if (val != 0)
            return val;
    }
    return 0;
}

const char *
lotto_module_kind_str(module_kind_t kind)
{
    static char buf[64];
    if (kind == MODULE_KIND_NONE) {
        return "NONE";
    }

    size_t off = 0;
    if (kind & MODULE_KIND_CLI) {
        off +=
            sys_snprintf(buf + off, sizeof(buf) - off, "%sCLI", off ? "|" : "");
    }
    if (kind & MODULE_KIND_ENGINE) {
        off += sys_snprintf(buf + off, sizeof(buf) - off, "%sENGINE",
                            off ? "|" : "");
    }
    if (kind & MODULE_KIND_RUNTIME) {
        off += sys_snprintf(buf + off, sizeof(buf) - off, "%sRUNTIME",
                            off ? "|" : "");
    }
    if (kind & MODULE_KIND_MEMMGR) {
        off += sys_snprintf(buf + off, sizeof(buf) - off, "%sMEMMGR",
                            off ? "|" : "");
    }
    if (off == 0) {
        return "UNKNOWN";
    }
    return buf;
}

void
lotto_module_print()
{
    logger_infof("Found %lu modules\n", _next);
    for (size_t i = 0; i < _next; ++i) {
        logger_infof(
            "Module '%s' %s found at '%s', shadowed = %s, disabled = %s\n",
            _modules[i].name, lotto_module_kind_str(_modules[i].kind),
            _modules[i].path, _modules[i].shadowed ? "yes" : "no",
            _modules[i].disabled ? "yes" : "no");
    }
}

static char *
_module_name(const char *filename)
{
    const char *name = filename;
    if (STARTS_WITH(filename, DRIVER_MODULE_PREFIX)) {
        name += DRIVER_MODULE_PREFIX_LEN;
    } else if (STARTS_WITH(filename, RUNTIME_VERBOSE_MODULE_PREFIX)) {
        name += RUNTIME_VERBOSE_MODULE_PREFIX_LEN;
    } else if (STARTS_WITH(filename, RUNTIME_MODULE_PREFIX)) {
        name += RUNTIME_MODULE_PREFIX_LEN;
    }
    size_t len = strrchr(name, '.') - name;
    return sys_strndup(name, len);
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
        if (!_ends_with(entry->d_name, SO_SUFFIX)) {
            continue;
        }
        char *name         = _module_name(entry->d_name);
        module_kind_t kind = MODULE_KIND_NONE;
        bool verbose       = false;
        if (STARTS_WITH(entry->d_name, DRIVER_MODULE_PREFIX)) {
            kind |= MODULE_KIND_CLI;
        }
        if (STARTS_WITH(entry->d_name, RUNTIME_VERBOSE_MODULE_PREFIX)) {
            kind |= MODULE_KIND_RUNTIME;
            verbose = true;
        } else if (STARTS_WITH(entry->d_name, RUNTIME_MODULE_PREFIX)) {
            kind |= MODULE_KIND_RUNTIME;
        }
        if (strstr(entry->d_name, "memmgr")) {
            kind |= MODULE_KIND_MEMMGR;
        }
        if (kind == MODULE_KIND_NONE) {
            logger_fatalf("unknown kind of module %s\n", entry->d_name);
        }
        for (size_t i = 0; i < _next; ++i) {
            if (!strcmp(name, _modules[i].name) && kind == _modules[i].kind &&
                verbose == _modules[i].verbose) {
                _modules[i].shadowed = true;
                _modules[i].disabled = true;
            }
        }
        sys_snprintf(path, PATH_MAX, "%s/%s", scan_dir, entry->d_name);
        module_t module = {.name     = name,
                           .path     = sys_strdup(path),
                           .kind     = kind,
                           .verbose  = verbose,
                           .shadowed = false,
                           .disabled = false};
        logger_debugf("Found new module '%s'\n", module.path);
        _modules[_next++] = module;
    }
    closedir(dir);
    return 0;
}

static int
_compar(const void *_p, const void *_q)
{
    const module_t *p = (const module_t *)_p;
    const module_t *q = (const module_t *)_q;
    int val;
    if (strstr(p->name, "rust")) {
        return 1;
    }
    if (strstr(q->name, "rust")) {
        return -1;
    }
    if ((val = strcmp(p->name, q->name)))
        return val;
    if ((val = (int)p->kind - (int)q->kind)) {
        return val;
    }
    return 0;
}
