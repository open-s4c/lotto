/*
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/envvar.h>
#include <lotto/base/marshable.h>
#include <lotto/base/trace_chunked.h>
#include <lotto/base/trace_flat.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/cli/args.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/sequencer.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stream_chunked_file.h>
#include <lotto/sys/stream_file.h>
#include <lotto/sys/string.h>
#include <sys/stat.h>
#include <sys/wait.h>

void
print_block(const char *s, size_t len)
{
    sys_fprintf(stdout, "%s", s);
    for (size_t i = 0; i + sys_strlen(s) < len; i++)
        sys_fprintf(stdout, " ");
}

/* https://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c
 */
int
cp(const char *from, const char *to)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0) {
        if (close(fd_to) < 0) {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

DECLARE_FLAG_ROUNDS;

void
round_print(const flags_t *flags, uint64_t round)
{
    char max[256];
    if (flags_get_uval(flags, FLAG_ROUNDS) == ~0UL)
        sys_sprintf(max, "inf");
    else
        sys_snprintf(max, 256, "%lu", flags_get_uval(flags, FLAG_ROUNDS));

    sys_fprintf(stdout, "[lotto] round: %lu/%s, %s\n", round, max,
                flags_get_sval(flags, flag_strategy()));
}

bool
adjust(const char *fn)
{
    bool result = false;
    if (fn == NULL || !fn[0])
        return result;
    /* load final record to automatically adjust configs */
    trace_t *t = cli_trace_load(fn);
    if (t) {
        record_t *final = trace_last(t);
        if (final && final->kind == RECORD_EXIT && final->size > 0) {
            statemgr_unmarshal(final->data, STATE_TYPE_FINAL, true);
            result = REASON_RUNTIME(final->reason);
        }
        trace_destroy(t);
    }
    return result;
}

uint64_t
get_file_hash(const char *pathname)
{
    /* Return hash of a file. Used for checking whether the same binary is
       used for record and replay. So far we just use size of the file, but
       it can be replaced with something more sophisticated if needed. */
    struct stat s;
    if (stat(pathname, &s) == -1) {
        /* Zero means that we were not able to calculate hash. */
        return 0;
    }
    return s.st_size;
}

uint64_t
get_lotto_hash(const char *arg0)
{
#if defined(__linux__)
    pid_t pid = getpid();
    char sname[1024], rname[1024] = {};
    sys_snprintf(sname, 1024, "/proc/%d/exe", pid);
    if (readlink(sname, rname, sizeof(rname)) < 0) {
        return get_file_hash(arg0);
    }
    return get_file_hash(rname);
#else
    return get_file_hash(arg0);
#endif
}

void
remove_all(const char *pat)
{
#if 0
    char cmd[256];
    sys_sprintf(cmd, "rm -f %s", pat);
    (void)system(cmd);
    #include <dirent.h>
    #include <stdio.h>

int main(void) {
  DIR *d;
  struct dirent *dir;
  d = opendir(".");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      sys_fprintf(stdout,"%s\n", dir->d_name);
    }
    closedir(d);
  }
  return(0);
}
#endif
}

static char temporary_directory_default[PATH_MAX];

const char *
get_default_temporary_directory()
{
    if (temporary_directory_default[0]) {
        return temporary_directory_default;
    }
    char *root = NULL;
    if (!(root = getenv("HOME"))) {
        root = getenv("PWD");
    }
    ASSERT(root);
    sys_sprintf(temporary_directory_default, "%s/.lotto", root);
    return temporary_directory_default;
}
