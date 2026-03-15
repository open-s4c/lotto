#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <lotto/driver/files.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/stdio.h>

static bool
driver_try_create_dir(const char *dir)
{
    struct stat st = {0};

    if (stat(dir, &st) != -1) {
        return true;
    }
    int r = mkdir(dir, 0755);
    if (r == 0) {
        return true;
    }
    return false;
}

bool
driver_try_dump_files(const char *dir, const driver_file_t files[])
{
    ASSERT(dir && "blob dir must be set");
    ASSERT(dir[0] == '/' && "blob dir path must be absolute");
    if (!driver_try_create_dir(dir)) {
        return false;
    }
    for (int i = 0; files[i].path; i++) {
        ASSERT(files[i].content);
        char filename[PATH_MAX];
        sys_sprintf(filename, "%s/%s", dir, files[i].path);
        FILE *fp = sys_fopen(filename, "w");
        if (fp == NULL) {
            return false;
        }
        if (sys_fwrite(files[i].content, sizeof(unsigned char), files[i].len,
                       fp) != files[i].len) {
            sys_fclose(fp);
            return false;
        }
        if (sys_fclose(fp) != 0) {
            return false;
        }
    }
    return true;
}

void
driver_dump_files(const char *dir, const driver_file_t files[])
{
    ASSERT(driver_try_dump_files(dir, files) &&
           "could not dump files, try specifying a different "
           "--temporary-directory CLI option");
}
