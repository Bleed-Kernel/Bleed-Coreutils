#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syscalls/mmap.h>

#define BUF_SIZE 4096

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("bad usage: cat <path>\n");
        exit(EXIT_FAILURE);
    }

    char targetpath[256];
    strncpy(targetpath, argv[1], sizeof(targetpath));
    FILE *f = fopen(targetpath, "rb");
    if (!f) {
        printf("cat: cannot open %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    void *buf = _mmap(BUF_SIZE);
    if ((uintptr_t)buf < 0x1000) {
        printf("cat: buffer allocation failed\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    while (1) {
        size_t n = fread(buf, 1, BUF_SIZE, f);
        if (n == 0) {
            break;
        }

        size_t w = fwrite(buf, 1, n, stdout);
        if (w != n) {
            printf("cat: write error\n");
            break;
        }
    }

    printf("\n");
    fclose(f);
    exit(EXIT_SUCCESS);
}
