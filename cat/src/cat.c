#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fs/file.h>

#define BUF_SIZE 4096

static int cat_fd(int fd) {
    char buf[BUF_SIZE];

    for (;;) {
        ssize_t r = read(fd, buf, sizeof(buf));
        if (r == 0)
            return 0;

        if (r < 0)
            return -1;

        ssize_t total = 0;
        while (total < r) {
            ssize_t w = write(STDOUT_FILENO, buf + total, r - total);
            if (w < 0)
                return -1;
            total += w;
        }
    }
}

int main(int argc, char **argv) {
    int status = 0;

    if (argc == 1) {
        if (cat_fd(STDIN_FILENO) < 0) {
            perror("cat");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    for (int i = 1; i < argc; ++i) {
        int fd;

        if (strcmp(argv[i], "-") == 0) {
            fd = STDIN_FILENO;
        } else {
            char path[PATH_MAX];
            size_t len = strlen(argv[i]);
            if (len == PATH_MAX) {
                fprintf(stderr, "cat: %s: File name too long\n", argv[i]);
                status = 1;
                continue;
            }
            memcpy(path, argv[i], len + 1);
            fd = open(path, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "cat: %s: %s\n",
                        argv[i], strerror(errno));
                status = 1;
                continue;
            }
        }

        if (cat_fd(fd) < 0) {
            fprintf(stderr, "cat: %s: %s\n",
                    strcmp(argv[i], "-") == 0 ? "stdin" : argv[i],
                    strerror(errno));
            status = 1;
        }

        if (fd != STDIN_FILENO)
            close(fd);
    }

    exit(status);
}