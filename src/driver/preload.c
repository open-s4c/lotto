#include <blob-tsano.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/cmake_variables.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(LOTTO_EMBED_LIB) || LOTTO_EMBED_LIB == 1
    #include <blob-lotto-verbose.h>
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
#include <lotto/sys/logger_block.h>
#include <lotto/sys/modules.h>
#include <lotto/sys/stdio.h>
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

#define LIBTSANO         "libtsano.so"
#define LIBTSAN0         "libtsan.so.0"
#define LIBTSAN2         "libtsan.so.2"
#define LIBCLANG_RT_TSAN "libclang_rt.tsan-x86_64.so"
typedef struct libspec {
    const char *filename;
    bool preload;
} libspec_t;

typedef bool (*module_predicate_f)(const module_t *, void *);

typedef struct preload_module_arg_s {
    module_predicate_f module_predicate;
    void *module_predicate_arg;
} preload_module_arg_t;

typedef struct module_preloadable_memory_arg_s {
    const char *name;
    uint64_t counter;
} module_preloadable_memory_arg_t;

static bool _is_runtime_plugin_path(const char *path);
static void _set_dice_plugin_modules_from_preload(void);

static const char *_libpath;
void
preload_set_libpath(const char *path)
{
    _libpath = path;
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
    const char *paths[] = {_libpath, dir, getenv("LD_LIBRARY_PATH")};
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
    setenv("LD_LIBRARY_PATH", ld_library_path, true);
}

static void
_preload_lib(const char *filename, bool preload_flag)
{
    if (!preload_flag) {
        return;
    }
    char ld_preload[MAX_LIST_STR];
    const char *var = getenv("LD_PRELOAD");
    if (var)
        sys_snprintf(ld_preload, MAX_LIST_STR, "%s:%s", var, filename);
    else
        sys_snprintf(ld_preload, MAX_LIST_STR, "%s", filename);
    setenv("LD_PRELOAD", ld_preload, true);
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
    if (module_predicate == NULL ||
        module_predicate(module, module_predicate_arg)) {
        _preload_lib(module->path, true);
    }
    return 0;
}

static void
_preload_libs(const char *dir, const libspec_t libspecs[])
{
#if defined(LOTTO_EMBED_LIB) && LOTTO_EMBED_LIB == 0
    for (const libspec_t *lib = libspecs; lib->filename; lib++) {
        _preload_lib(lib->filename, lib->preload);
    }
#else
    char path[PATH_MAX];
    for (const libspec_t *lib = libspecs; lib->filename; lib++) {
        sys_sprintf(path, "%s/%s", dir, lib->filename);
        _preload_lib(path, lib->preload);
    }
#endif
}

static void
_preload_memmgr_plugins(const char *chain, bool runtime)
{
    if (chain == NULL || chain[0] == '\0') {
        return;
    }
    char chain_cpy[MAX_LIST_STR];
    strcpy(chain_cpy, chain);
    for (const char *i = strtok(chain_cpy, ":"); i != NULL;
         i             = strtok(NULL, ":")) {
        char name[MAX_LIST_STR];
        sys_sprintf(name, "%s_%s", runtime ? "runtime" : "user", i);
        module_preloadable_memory_arg_t arg =
            (module_preloadable_memory_arg_t){.name = name};
        lotto_module_foreach_reverse(
            _preload_module, &(preload_module_arg_t){
                                 .module_predicate = module_preloadable_memory,
                                 .module_predicate_arg = &arg});
        //        ENSURE(arg.counter == 1 && "could not load a memory plugin");
    }
}

static bool
_is_runtime_plugin_path(const char *path)
{
    const char *base = strrchr(path, '/');
    base             = base ? base + 1 : path;
    return strncmp(base, "lotto-runtime-", strlen("lotto-runtime-")) == 0;
}

static void
_set_dice_plugin_modules_from_preload(void)
{
    const char *ld_preload = getenv("LD_PRELOAD");
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
    if (len > 0) {
        setenv(DICE_PLUGIN_MODULES, plugin_modules, true);
    } else {
        unsetenv(DICE_PLUGIN_MODULES);
    }
}

void
preload(const char *dir, bool verbose, bool do_preload_plotto,
        const char *memmgr_chain_runtime, const char *memmgr_chain_user)
{
    const char *cli_preload = getenv(LOTTO_CLI_PRELOAD);

    /* make libraries available */
    // clang-format off
    driver_dump_files(dir, (driver_file_t[]) {
        {.path    = LIBTSANO,
         .content = libtsano_so,
         .len     = libtsano_so_len},
#if !defined(LOTTO_EMBED_LIB) || LOTTO_EMBED_LIB == 1
        verbose ?
        (driver_file_t){.path    = LIBLOTTO,
                        .content = liblotto_runtime_verbose_so,
                        .len     = liblotto_runtime_verbose_so_len} :
        (driver_file_t){.path    = LIBLOTTO,
                        .content = liblotto_runtime_so,
                        .len     = liblotto_runtime_so_len},
#endif
        {NULL}});
    // clang-format on

    _symlink_lib(dir, LIBTSANO,
                 (const char *[]){LIBTSAN0, LIBTSAN2, LIBCLANG_RT_TSAN, NULL});
    /* preload libraries */

    const char *logger_level = verbose ? "debug" : "error";
    envvar_t vars[]          = {{"LOTTO_LOGGER_LEVEL", logger_level},
                                {"LOTTO_TEMP_DIR", dir},
                                {NULL}};
    envvar_set(vars, true);

    _set_libpath_env(dir);

    if (exec_info_replay_envvars()) {
        return;
    }

    if (cli_preload && cli_preload[0]) {
        setenv("LD_PRELOAD", cli_preload, true);
    } else {
        unsetenv("LD_PRELOAD");
    }

#ifdef __SANITIZE_ADDRESS__
    _preload_lib(LIBASAN, true);
#endif
    /* preload memmgr chains */
    _preload_memmgr_plugins(memmgr_chain_runtime, true);
    _preload_memmgr_plugins(memmgr_chain_user, false);

    /* preload the runtime library */
    _preload_libs(dir, (libspec_t[]){
                           {LIBLOTTO, do_preload_plotto},
                           {NULL},
                       });

    /* preload other dynamic modules */
    if (do_preload_plotto) {
        lotto_module_foreach_reverse(
            _preload_module,
            &(preload_module_arg_t){.module_predicate =
                                        module_preloadable_not_memory});
    }

    _set_dice_plugin_modules_from_preload();

    exec_info_store_envvars();
}
