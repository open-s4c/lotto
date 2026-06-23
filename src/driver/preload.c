#include <blob-tsano.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/cmake_variables.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(LOTTO_EMBED_LIB) || LOTTO_EMBED_LIB == 1
    #include <blob-lotto-dbg.h>
    #include <blob-lotto.h>
#endif

#include <lotto/base/envvar.h>
#include <lotto/base/libraries.h>
#include <lotto/cmake_variables.h>
#include <lotto/driver/exec_info.h>
#include <lotto/driver/files.h>
#include <lotto/driver/preload.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/modules.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PLUGIN_PATHS                                                           \
    LOTTO_MODULE_BUILD_DIR                                                     \
    ":" LOTTO_MODULE_INSTALL_DIR ":" QLOTTO_MODULE_BUILD_DIR                   \
    ":" QLOTTO_MODULE_INSTALL_DIR ":" CMAKE_INSTALL_PREFIX                     \
    "/share/lotto"                                                             \
    ":" CMAKE_BINARY_DIR

#define MAX_LIST_STR        ((size_t)(32 * 1024))
#define DICE_PLUGIN_MODULES "DICE_PLUGIN_MODULES"
#define LOTTO_CLI_PRELOAD   "LOTTO_CLI_PRELOAD"
#define LOTTO_LOAD_RUNTIME  "LOTTO_LOAD_RUNTIME"

#if defined(__APPLE__)
#    define LOTTO_PRELOAD_ENV "DYLD_INSERT_LIBRARIES"
#    define LOTTO_LIBPATH_ENV "DYLD_LIBRARY_PATH"
#else
#    define LOTTO_PRELOAD_ENV "LD_PRELOAD"
#    define LOTTO_LIBPATH_ENV "LD_LIBRARY_PATH"
#endif

#if defined(__APPLE__)
#    define LIBTSANO         "libtsano.dylib"
#    define LIBCLANG_RT_TSAN "libclang_rt.tsan_osx_dynamic.dylib"
#    define LIBRUSTC_NIGHTLY_RT_TSAN "librustc-nightly_rt.tsan.dylib"
#    define LIBRUSTC_NIGHTLY_RT_TSAN_COMPAT \
        "librustc-nightly_rt.tsan_osx_dynamic.dylib"
#    define BLOB_LIBTSANO    libtsano_dylib
#    define BLOB_LIBTSANO_LEN libtsano_dylib_len
#    define BLOB_LIBLOTTO     liblotto_runtime_dylib
#    define BLOB_LIBLOTTO_LEN liblotto_runtime_dylib_len
#    define BLOB_LIBLOTTO_DBG liblotto_runtime_dbg_dylib
#    define BLOB_LIBLOTTO_DBG_LEN liblotto_runtime_dbg_dylib_len
#else
#    define LIBTSANO         "libtsano.so"
#    define LIBCLANG_RT_TSAN "libclang_rt.tsan-x86_64.so"
#    define LIBRUSTC_NIGHTLY_RT_TSAN "librustc-nightly_rt.tsan-x86_64.so"
#    define LIBRUSTC_NIGHTLY_RT_TSAN_COMPAT \
        "librustc-nightly_rt.tsan-x86_64.so"
#    define BLOB_LIBTSANO    libtsano_so
#    define BLOB_LIBTSANO_LEN libtsano_so_len
#    define BLOB_LIBLOTTO     liblotto_runtime_so
#    define BLOB_LIBLOTTO_LEN liblotto_runtime_so_len
#    define BLOB_LIBLOTTO_DBG liblotto_runtime_dbg_so
#    define BLOB_LIBLOTTO_DBG_LEN liblotto_runtime_dbg_so_len
#endif

#define LIBTSAN0         "libtsan.so.0"
#define LIBTSAN2         "libtsan.so.2"
typedef struct libspec {
    const char *filename;
    bool preload;
} libspec_t;

typedef bool (*module_predicate_f)(const module_t *, void *);

typedef struct preload_module_arg_s {
    module_predicate_f module_predicate;
    void *module_predicate_arg;
    bool runtime_dbg;
} preload_module_arg_t;

typedef struct module_preloadable_memory_arg_s {
    const char *name;
    uint64_t counter;
} module_preloadable_memory_arg_t;

static bool _is_runtime_plugin_path(const char *path);
static void _preload_list(const char *paths);
static void _append_path_list(char *buf, size_t buf_size, size_t *len,
                              const char *paths);
static void _set_dice_plugin_modules_from_preload(void);
static const char *_resolve_runtime_path(const char *path, char *buf,
                                         size_t buf_size);

static const char *_libpath;
void
preload_set_libpath(const char *path)
{
    _libpath = path;
}

static const char *
_resolve_runtime_path(const char *path, char *buf, size_t buf_size)
{
#if defined(__APPLE__)
    struct stat st = {0};
    size_t len     = strlen(path);
    if (len > 3 && strcmp(path + len - 3, ".so") == 0 && stat(path, &st) != 0) {
        int written =
            snprintf(buf, buf_size, "%.*s.dylib", (int)(len - 3), path);
        ASSERT(written >= 0 && (size_t)written < buf_size);
        if (stat(buf, &st) == 0) {
            return buf;
        }
    }
#else
    (void)buf;
    (void)buf_size;
#endif
    return path;
}

static void
_create_dir(const char *dir)
{
    struct stat st = {0};

    if (stat(dir, &st) != -1) {
        return;
    }
    int r = mkdir(dir, 0755);
    if (r == 0) {
        return;
    }
    sys_fprintf(stdout, "%d != 0, could not create %s dir", r, dir);
    sys_abort();
}

static void
_symlink_lib(const char *dir, const char *lib, const char *links[])
{
    char filename[PATH_MAX];
    sys_sprintf(filename, "%s/%s", dir, lib);
    struct stat st = {0};

    for (size_t i = 0; links[i]; i++) {
        char linkname[PATH_MAX];
        sys_sprintf(linkname, "%s/%s", dir, links[i]);
        if (stat(linkname, &st) != -1) {
            /* delete symlink */
            ENSURE(remove(linkname) == 0 &&
                   "could not remove symlink, try specifying a different "
                   "--temporary-directory CLI option");
        }
        ENSURE(symlink(filename, linkname) == 0 &&
               "could not symlink, try specifying a different "
               "--temporary-directory CLI option");
    }
}

static void
_set_libpath_env(const char *dir)
{
    char ld_library_path[MAX_LIST_STR];
    const char *paths[] = {_libpath, dir, getenv(LOTTO_LIBPATH_ENV)};
    bool first          = true;
    size_t b            = 0;
    for (size_t i = 0; i < sizeof(paths) / sizeof(char *); i++) {
        if (!paths[i]) {
            continue;
        }
        b += sys_snprintf(ld_library_path + b, MAX_LIST_STR - b,
                          first ? "%s" : ":%s", paths[i]);
        first = false;
    }
    setenv(LOTTO_LIBPATH_ENV, ld_library_path, true);
}

static void
_preload_lib(const char *filename, bool preload_flag)
{
    if (!preload_flag) {
        return;
    }
    char ld_preload[MAX_LIST_STR];
    const char *var = getenv(LOTTO_PRELOAD_ENV);
    if (var)
        sys_snprintf(ld_preload, MAX_LIST_STR, "%s:%s", var, filename);
    else
        sys_snprintf(ld_preload, MAX_LIST_STR, "%s", filename);
    setenv(LOTTO_PRELOAD_ENV, ld_preload, true);
}

static bool
module_preloadable_not_memory(const module_t *module, void *arg)
{
    (void)arg;
    return module->kind & (MODULE_KIND_RUNTIME | MODULE_KIND_ENGINE) &&
           !(module->kind & MODULE_KIND_MEMMGR);
}

static bool
module_preloadable_memory(const module_t *module, void *arg)
{
    module_preloadable_memory_arg_t *args =
        (module_preloadable_memory_arg_t *)arg;
    if (!(module->kind & (MODULE_KIND_RUNTIME | MODULE_KIND_ENGINE) &&
          module->kind & MODULE_KIND_MEMMGR) ||
        strstr(module->name, args->name) == NULL) {
        return false;
    }
    args->counter++;
    return true;
}

static int
_preload_module(module_t *module, void *arg)
{
    preload_module_arg_t *args          = (preload_module_arg_t *)arg;
    module_predicate_f module_predicate = args->module_predicate;
    void *module_predicate_arg          = args->module_predicate_arg;
    if ((module->kind & MODULE_KIND_RUNTIME) &&
        module->dbg != args->runtime_dbg) {
        return 0;
    }
    if (module_predicate == NULL ||
        module_predicate(module, module_predicate_arg)) {
        _preload_lib(module->path, true);
    }
    return 0;
}

static void
_preload_libs(const char *dir, const libspec_t libspecs[])
{
    char path[PATH_MAX];
    for (const libspec_t *lib = libspecs; lib->filename; lib++) {
#if defined(LOTTO_EMBED_LIB) && LOTTO_EMBED_LIB == 0
        const char *base = _libpath ? _libpath : dir;
#else
        const char *base = dir;
#endif
        sys_sprintf(path, "%s/%s", dir, lib->filename);
        if (base != dir) {
            sys_sprintf(path, "%s/%s", base, lib->filename);
        }
        _preload_lib(path, lib->preload);
    }
}

static void
_preload_memmgr_plugins(const char *chain, bool runtime, bool dbg)
{
    if (chain == NULL || chain[0] == '\0') {
        return;
    }
    char chain_cpy[MAX_LIST_STR];
    strcpy(chain_cpy, chain);
    for (const char *i = strtok(chain_cpy, ":"); i != NULL;
         i             = strtok(NULL, ":")) {
        char name[MAX_LIST_STR];
        const char *side = runtime ? "runtime" : "user";
        if (strcmp(i, "uaf") == 0) {
            sys_sprintf(name, "uafcheck-memmgr_%s", side);
        } else if (strcmp(i, "leakcheck") == 0) {
            sys_sprintf(name, "leakcheck-memmgr_%s", side);
        } else if (strcmp(i, "mempool") == 0) {
            sys_sprintf(name, "user_mempool-memmgr_%s", side);
        } else {
            sys_sprintf(name, "%s_%s", side, i);
        }
        module_preloadable_memory_arg_t arg =
            (module_preloadable_memory_arg_t){.name = name};
        lotto_module_foreach_reverse(
            _preload_module, &(preload_module_arg_t){
                                 .module_predicate = module_preloadable_memory,
                                 .module_predicate_arg = &arg,
                                 .runtime_dbg          = dbg});
        ENSURE(arg.counter == 1 && "could not load a memory plugin");
    }
}

static bool
_parse_runtime_plugin_path(const char *path, const char **name,
                           const char **dir)
{
    static char out_dir[MAX_LIST_STR]  = {0};
    static char out_name[MAX_LIST_STR] = {0};
    if (dir)
        *dir = out_dir;
    if (name)
        *name = out_name;

    const char *base = strrchr(path, '/');
    if (base) {
        strncpy(out_dir, path, (uintptr_t)base - (uintptr_t)path);
        base += 1;
    } else {
        out_dir[0] = 0;
    }
    if (strncmp(base, RUNTIME_DBG_MODULE_PREFIX,
                RUNTIME_DBG_MODULE_PREFIX_LEN) == 0) {
        strncpy(out_name, base + RUNTIME_DBG_MODULE_PREFIX_LEN, MAX_LIST_STR);
        return true;
    } else if (strncmp(base, RUNTIME_MODULE_PREFIX,
                       RUNTIME_MODULE_PREFIX_LEN) == 0) {
        strncpy(out_name, base + RUNTIME_MODULE_PREFIX_LEN, MAX_LIST_STR);
        return true;
    } else {
        return false;
    }
}

static bool
_is_runtime_plugin_path(const char *path)
{
    const char *base = strrchr(path, '/');
    base             = base ? base + 1 : path;
    return strncmp(base, "lotto-runtime-", strlen("lotto-runtime-")) == 0 ||
           strcmp(base, LIBLOTTO_RUNTIME) == 0 ||
           strcmp(base, LIBLOTTO_RUNTIME_DBG) == 0;
}

static void
_preload_list(const char *paths)
{
    char paths_copy[MAX_LIST_STR];

    if (paths == NULL || paths[0] == '\0') {
        return;
    }

    sys_snprintf(paths_copy, sizeof(paths_copy), "%s", paths);
    for (const char *path = strtok(paths_copy, ":"); path != NULL;
         path             = strtok(NULL, ":")) {
        char resolved[MAX_LIST_STR];
        _preload_lib(_resolve_runtime_path(path, resolved, sizeof(resolved)),
                     true);
    }
}

static void
_append_path_list(char *buf, size_t buf_size, size_t *len, const char *paths)
{
    char paths_copy[MAX_LIST_STR];

    if (paths == NULL || paths[0] == '\0') {
        return;
    }

    sys_snprintf(paths_copy, sizeof(paths_copy), "%s", paths);
    for (const char *path = strtok(paths_copy, ":"); path != NULL;
         path             = strtok(NULL, ":")) {
        char resolved[MAX_LIST_STR];
        const char *entry = _resolve_runtime_path(path, resolved, sizeof(resolved));
        int written;
        if (*len > 0) {
            written = sys_snprintf(buf + *len, buf_size - *len, ":%s", entry);
        } else {
            written = sys_snprintf(buf + *len, buf_size - *len, "%s", entry);
        }
        ASSERT(written >= 0 && (size_t)written < buf_size - *len);
        *len += (size_t)written;
    }
}

static void
_set_dice_plugin_modules_from_preload(void)
{
    const char *ld_preload    = getenv(LOTTO_PRELOAD_ENV);
    const char *runtime_loads = getenv(LOTTO_LOAD_RUNTIME);
    char plugin_modules[MAX_LIST_STR];
    size_t len = 0;

    plugin_modules[0] = '\0';
    if (ld_preload && ld_preload[0]) {
        char ld_preload_cpy[MAX_LIST_STR];
        sys_snprintf(ld_preload_cpy, sizeof(ld_preload_cpy), "%s", ld_preload);
        for (const char *path = strtok(ld_preload_cpy, ":"); path != NULL;
             path             = strtok(NULL, ":")) {
            int written;
            if (!_is_runtime_plugin_path(path))
                continue;
            if (len > 0) {
                written =
                    sys_snprintf(plugin_modules + len,
                                 sizeof(plugin_modules) - len, ":%s", path);
            } else {
                written =
                    sys_snprintf(plugin_modules + len,
                                 sizeof(plugin_modules) - len, "%s", path);
            }
            ASSERT(written >= 0 &&
                   (size_t)written < sizeof(plugin_modules) - len);
            len += (size_t)written;
        }
    }
    _append_path_list(plugin_modules, sizeof(plugin_modules), &len,
                      runtime_loads);
    if (len > 0) {
        setenv(DICE_PLUGIN_MODULES, plugin_modules, true);
    } else {
        unsetenv(DICE_PLUGIN_MODULES);
    }
}

/* Check if the trace was generated with liblotto-dbg. */
bool
_ld_preload_is_dbg()
{
    const char *ld_preload = getenv(LOTTO_PRELOAD_ENV);
    return ld_preload && strstr(ld_preload, LIBLOTTO_RUNTIME_DBG);
}

/* Replace runtime plugin paths with the correct paths:
 * if the user requested dbg, replace them with dbg versions;
 * otherwise, replace them with ndbg versions. */
void
_ld_preload_fix_dbg(bool is_dbg)
{
    const char *ld_preload = getenv(LOTTO_PRELOAD_ENV);
    if (!ld_preload || ld_preload[0] == 0)
        return;
    char ld_preload_cpy[MAX_LIST_STR];
    char result[MAX_LIST_STR] = {0};
    size_t len                = 0;
    sys_strcpy(ld_preload_cpy, ld_preload);
    for (const char *path = strtok(ld_preload_cpy, ":");
         path != NULL && len < sizeof(result) - 1; path = strtok(NULL, ":")) {
        char module_path[MAX_LIST_STR] = {0};
        const char *module_name = NULL, *module_dir = NULL;
        if (!sys_strcmp(path, LIBLOTTO_RUNTIME) && is_dbg) {
            strncpy(result + len, LIBLOTTO_RUNTIME_DBG,
                    sizeof(result) - len - 1);
            len += sys_strlen(LIBLOTTO_RUNTIME_DBG);
            result[len++] = ':';
        } else if (!sys_strcmp(path, LIBLOTTO_RUNTIME_DBG) && !is_dbg) {
            strncpy(result + len, LIBLOTTO_RUNTIME, sizeof(result) - len - 1);
            len += sys_strlen(LIBLOTTO_RUNTIME);
            result[len++] = ':';
        } else if (_parse_runtime_plugin_path(path, &module_name,
                                              &module_dir)) {
            snprintf(module_path, MAX_LIST_STR, "%s/lotto-runtime%s-%s",
                     module_dir, is_dbg ? "-dbg" : "", module_name);
            strncpy(result + len, module_path, sizeof(result) - len - 1);
            len += sys_strlen(module_path);
            result[len++] = ':';
        }
    }
    result[--len] = 0;
    setenv(LOTTO_PRELOAD_ENV, result, true);
}

void
preload(const char *dir, uint64_t verbose, bool do_preload_plotto,
        const char *memmgr_chain_runtime, const char *memmgr_chain_user)
{
    const char *cli_preload = getenv(LOTTO_CLI_PRELOAD);

    /* make libraries available */
    // clang-format off
    driver_dump_files(dir, (driver_file_t[]) {
        {.path    = LIBTSANO,
         .content = BLOB_LIBTSANO,
         .len     = BLOB_LIBTSANO_LEN},
#if !defined(LOTTO_EMBED_LIB) || LOTTO_EMBED_LIB == 1
        verbose > 0 ?
        (driver_file_t){.path    = LIBLOTTO_RUNTIME_DBG,
                        .content = BLOB_LIBLOTTO_DBG,
                        .len     = BLOB_LIBLOTTO_DBG_LEN} :
        (driver_file_t){.path    = LIBLOTTO_RUNTIME,
                        .content = BLOB_LIBLOTTO,
                        .len     = BLOB_LIBLOTTO_LEN},
#endif
        {NULL}});
    // clang-format on

#if defined(__APPLE__)
    _symlink_lib(dir, LIBTSANO,
                 (const char *[]){LIBCLANG_RT_TSAN, LIBRUSTC_NIGHTLY_RT_TSAN,
                                  LIBRUSTC_NIGHTLY_RT_TSAN_COMPAT,
                                  NULL});
#else
    _symlink_lib(dir, LIBTSANO,
                 (const char *[]){LIBTSAN0, LIBTSAN2, LIBCLANG_RT_TSAN,
                                  LIBRUSTC_NIGHTLY_RT_TSAN,
                                  LIBRUSTC_NIGHTLY_RT_TSAN_COMPAT, NULL});
#endif
    /* preload libraries */

    const char *logger_level = verbose >= 2 ? "debug" :
                               verbose >= 1 ? "info" :
                                              "error";
    envvar_t vars[]          = {{"LOTTO_LOGGER_LEVEL", logger_level},
                                {"LOTTO_TEMP_DIR", dir},
                                {NULL}};
    envvar_set(vars, true);

    _set_libpath_env(dir);

    ASSERT(verbose <= INT_MAX);
    bool replayed = exec_info_replay_envvars((int)verbose);
    bool is_dbg   = verbose > 0;
    if (replayed && _ld_preload_is_dbg() == is_dbg) {
        return;
    } else if (replayed) {
        _ld_preload_fix_dbg(is_dbg);
        _set_dice_plugin_modules_from_preload();
        return;
    }

    if (cli_preload && cli_preload[0]) {
        setenv(LOTTO_PRELOAD_ENV, cli_preload, true);
    } else {
        unsetenv(LOTTO_PRELOAD_ENV);
    }

#ifdef __SANITIZE_ADDRESS__
    _preload_lib(LIBASAN, true);
#endif
    /* preload memmgr chains */
    _preload_memmgr_plugins(memmgr_chain_runtime, true, verbose > 0);
    _preload_memmgr_plugins(memmgr_chain_user, false, verbose > 0);

    /* preload the runtime library */
    _preload_libs(dir,
                  (libspec_t[]){
                      {verbose > 0 ? LIBLOTTO_RUNTIME_DBG : LIBLOTTO_RUNTIME,
                       do_preload_plotto},
                      {NULL},
                  });

    /* explicit runtime loads append after the Lotto runtime for now */
    _preload_list(getenv(LOTTO_LOAD_RUNTIME));

    /* preload other dynamic modules */
    if (do_preload_plotto) {
        lotto_module_foreach_reverse(
            _preload_module,
            &(preload_module_arg_t){.module_predicate =
                                        module_preloadable_not_memory,
                                    .runtime_dbg = verbose > 0});
    }
    _set_dice_plugin_modules_from_preload();

    exec_info_store_envvars();
}
