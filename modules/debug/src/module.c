#include <blob-command.h>
#include <blob-debug.h>
#include <blob-decorator.h>
#include <blob-filter.h>
#include <blob-handler.h>
#include <blob-iterator.h>
#include <blob-matcher.h>
#include <blob-util.h>
#include <limits.h>

#include "debug.h"
#include <dice/module.h>
#include <lotto/driver/files.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/stdio.h>

#define GDB_PYTHON_DIR "pylotto"

bool
debug_dump_assets(const char *dir)
{
    bool ok = true;
    ok &= driver_try_dump_files(
        dir,
        (driver_file_t[]){
            {.path = "debug.gdb", .content = debug_gdb, .len = debug_gdb_len},
            {NULL},
        });

    char python_dir[PATH_MAX];
    sys_sprintf(python_dir, "%s/%s", dir, GDB_PYTHON_DIR);
    ok &= driver_try_dump_files(
        python_dir,
        (driver_file_t[]){
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
    return ok;
}

ON_INITIALIZATION_PHASE({
    (void)debug_dump_assets(get_default_temporary_directory());
})
