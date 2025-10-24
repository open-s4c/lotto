// clang-format off
// UNSUPPORTED: aarch64
// XFAIL: *
// RUN: exit 1
// RUN: %cleanup
// RUN: %b > %T/output1.log
// RUN: %b > %T/output2.log
// RUN: diff %T/output1.log %T/output2.log
// clang-format on
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int
main()
{
    const char *filename = "text-file.dat";
    {
        FILE *fp = fopen(filename, "r+");
        if (fp) {
            char msg[128];
            size_t r = fread(msg, 1, 128, fp);
            printf("read: %lu bytes\n", r);
            printf("data: %s\n", msg);
            fclose(fp);
        } else {
            printf("file does not exist\n");
        }
    }

    {
        FILE *fp   = fopen(filename, "w+");
        char msg[] = "Hello world";
        size_t s   = fwrite(msg, 1, strlen(msg) + 1, fp);
        printf("written: %lu bytes\n", s);
        printf("data: %s\n", msg);
        fclose(fp);
    }

    return 0;
}
