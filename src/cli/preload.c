#include <blob-command.h>
#include <blob-crep.h>
#include <blob-debug.h>
#include <blob-decorator.h>
#include <blob-engine.h>
#include <blob-filter.h>
#include <blob-handler.h>
#include <blob-iterator.h>
#include <blob-matcher.h>
#include <blob-runtime.h>
#include <blob-tsano.h>
#include <blob-util.h>
#include <dirent.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/cmake_variables.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(LOTTO_EMBED_LIB) || LOTTO_EMBED_LIB == 1
    #include <blob-flotto.h>
    #include <blob-plotto-verbose.h>
    #include <blob-plotto.h>
#endif

#include <lotto/base/envvar.h>
#include <lotto/cli/exec_info.h>
#include <lotto/cli/preload.h>
#include <lotto/engine/handlers/ichpt.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/plugin.h>
#include <lotto/sys/stdio.h>

#define PLUGIN_PATHS                                                           \
    LOTTO_PLUGIN_BUILD_DIR                                                     \
    ":" LOTTO_PLUGIN_INSTALL_DIR ":" QLOTTO_PLUGIN_BUILD_DIR                   \
    ":" QLOTTO_PLUGIN_INSTALL_DIR ":" CMAKE_INSTALL_PREFIX                     \
    "/share/lotto"                                                             \
    ":" CMAKE_BINARY_DIR

#define MAX_LIST_STR ((size_t)(32 * 1024))

typedef struct blob_s {
    const char *path;
    const unsigned char *content;
    unsigned int len;
} blob_t;

typedef struct libspec {
    const char *filename;
    bool preload;
} libspec_t;

typedef bool (*plugin_predicate_f)(const plugin_t *, void *);

typedef struct preload_plugin_arg_s {
    plugin_predicate_f plugin_predicate;
    void *plugin_predicate_arg;
} preload_plugin_arg_t;

typedef struct plugin_preloadable_memory_arg_s {
    const char *name;
    uint64_t counter;
} plugin_preloadable_memory_arg_t;

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
_fwrite_blob(const char *dir, const char *filename,
             const unsigned char content[], unsigned int content_len, ...)
{
    ASSERT(content[content_len - 1] == '\0' &&
           "format blob must terminate with null character");
    ASSERT(dir && "blob dir must be set");
    ASSERT(dir[0] == '/' && "blob dir path must be absolute");
    _create_dir(dir);
    char filepath[PATH_MAX];
    sys_sprintf(filepath, "%s/%s", dir, filename);
    FILE *fp = sys_fopen(filepath, "w");
    ASSERT(fp &&
           "could not open a file for writing a blob, try "
           "specifying a different --temporary-directory CLI option");
    va_list args;
    va_start(args, content_len);
    sys_vfprintf(fp, (char *)content, args);
    sys_fclose(fp);
}

static void
_write_blobs(const char *dir, const blob_t blobs[])
{
    ASSERT(dir && "blob dir must be set");
    ASSERT(dir[0] == '/' && "blob dir path must be absolute");
    _create_dir(dir);
    for (int i = 0; blobs[i].path; i++) {
        ASSERT(blobs[i].content);
        char filename[PATH_MAX];
        sys_sprintf(filename, "%s/%s", dir, blobs[i].path);
        FILE *fp = sys_fopen(filename, "w");
        ASSERT(fp &&
               "could not open a file for writing a blob, try "
               "specifying a different --temporary-directory CLI option");
        ENSURE(sys_fwrite(blobs[i].content, sizeof(unsigned char), blobs[i].len,
                          fp) == blobs[i].len &&
               "could not write blob to the file, try specifying a different "
               "--temporary-directory CLI option");
        ENSURE(sys_fclose(fp) == 0 &&
               "could not close the blob file, try specifying a different "
               "--temporary-directory CLI option");
    }
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
plugin_preloadable_not_memory(const plugin_t *plugin, void *arg)
{
    (void)arg;
    return plugin->kind & (PLUGIN_KIND_RUNTIME | PLUGIN_KIND_ENGINE) &&
           !(plugin->kind & PLUGIN_KIND_MEMMGR);
}

static bool
plugin_preloadable_memory(const plugin_t *plugin, void *arg)
{
    plugin_preloadable_memory_arg_t *args =
        (plugin_preloadable_memory_arg_t *)arg;
    if (!(plugin->kind & (PLUGIN_KIND_RUNTIME | PLUGIN_KIND_ENGINE) &&
          plugin->kind & PLUGIN_KIND_MEMMGR) ||
        strstr(plugin->name, args->name) == NULL) {
        return false;
    }
    args->counter++;
    return true;
}

static int
_preload_plugin(plugin_t *plugin, void *arg)
{
    preload_plugin_arg_t *args          = (preload_plugin_arg_t *)arg;
    plugin_predicate_f plugin_predicate = args->plugin_predicate;
    void *plugin_predicate_arg          = args->plugin_predicate_arg;
    if (plugin_predicate == NULL ||
        plugin_predicate(plugin, plugin_predicate_arg)) {
        _preload_lib(plugin->path, true);
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
        sys_sprintf(name, "%s_%s", i, runtime ? "runtime" : "user");
        plugin_preloadable_memory_arg_t arg =
            (plugin_preloadable_memory_arg_t){.name = name};
        lotto_plugin_foreach_reverse(
            _preload_plugin, &(preload_plugin_arg_t){
                                 .plugin_predicate = plugin_preloadable_memory,
                                 .plugin_predicate_arg = &arg});
        ENSURE(arg.counter == 1 && "could not load a memory plugin");
    }
}

void
preload(const char *dir, bool verbose, bool do_preload_plotto,
        bool do_preload_crep, bool flotto, const char *memmgr_chain_runtime,
        const char *memmgr_chain_user)
{
    /* make libraries available */
    // clang-format off
    _write_blobs(dir, (blob_t[]) {
        {.path    = LIBRUNTIME,
         .content = libruntime_so,
         .len     = libruntime_so_len},
        {.path    = LIBTSANO,
         .content = libtsano_so,
         .len     = libtsano_so_len},
        flotto ?
        (blob_t){.path    = LIBENGINE,
                 .content = libflotto_so,
                 .len     = libflotto_so_len} :
        (blob_t){.path    = LIBENGINE,
                 .content = libengine_so,
                 .len     = libengine_so_len},
#if !defined(LOTTO_EMBED_LIB) || LOTTO_EMBED_LIB == 1
        false ?
        (blob_t){.path    = LIBLOTTO,
                 .content = libplotto_verbose_so,
                 .len     = libplotto_verbose_so_len} :
        (blob_t){.path    = LIBLOTTO,
                 .content = libplotto_so,
                 .len     = libplotto_so_len},
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

#ifdef __SANITIZE_ADDRESS__
    _preload_lib(LIBASAN, true);
#endif

    /* Load three dirs in order. */
    _preload_memmgr_plugins(memmgr_chain_runtime, true);
    _preload_memmgr_plugins(memmgr_chain_user, false);
    if (do_preload_plotto && !flotto) {
        lotto_plugin_foreach_reverse(
            _preload_plugin,
            &(preload_plugin_arg_t){.plugin_predicate =
                                        plugin_preloadable_not_memory});
    }

    _preload_libs(dir, (libspec_t[]){
                           {LIBLOTTO, do_preload_plotto},
                           {LIBRUNTIME, do_preload_plotto},
                           {LIBENGINE, do_preload_plotto},
                           {LIBCREP, do_preload_crep},
                           {NULL},
                       });
    exec_info_store_envvars();
}

void
debug_preload(const char *dir, const char *file_filter,
              const char *function_filter, const char *addr2line,
              const char *symbol_file)
{
    _fwrite_blob(dir, "debug.gdb", debug_gdb, debug_gdb_len, dir, file_filter,
                 function_filter, addr2line, symbol_file, PLUGIN_PATHS);
    char python_dir[PATH_MAX];
    sys_sprintf(python_dir, "%s/%s", dir, GDB_PYTHON_DIR);
    _write_blobs(
        python_dir,
        (blob_t[]){
            {.path    = "command.py",
             .content = command_py,
             .len     = command_py_len},
            {.path    = "decorator.py",
             .content = decorator_py,
             .len     = decorator_py_len},
            {.path = "filter.py", .content = filter_py, .len = filter_py_len},
            {.path    = "iterator.py",
             .content = iterator_py,
             .len     = iterator_py_len},
            {.path = "util.py", .content = util_py, .len = util_py_len},
            {.path    = "matcher.py",
             .content = matcher_py,
             .len     = matcher_py_len},
            {.path    = "handler.py",
             .content = handler_py,
             .len     = handler_py_len},
            {NULL},
        });
}
