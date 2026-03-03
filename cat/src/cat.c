#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fs/file.h>
#include <ipc.h>
#include <syscalls/munmap.h>

#define BUF_SIZE 4096
#define IPC_PAGE_SIZE 4096UL

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

static int cat_ipc_if_present(void) {
    ipc_message_t msg;
    if (ipc_recv(&msg) < 0)
        return 0;

    const char *payload = (const char *)(uintptr_t)msg.addr;
    size_t max = (size_t)msg.pages * IPC_PAGE_SIZE;
    size_t n = 0;
    while (n < max && payload[n] != '\0')
        n++;

    if (n > 0) {
        ssize_t w = write(STDOUT_FILENO, payload, n);
        if (w < 0 || (size_t)w != n) {
            _munmap((void *)(uintptr_t)msg.addr);
            return -1;
        }
    }

    _munmap((void *)(uintptr_t)msg.addr);
    return 0;
}

int main(int argc, char **argv) {
    int status = 0;
    if (cat_ipc_if_present() < 0) {
        perror("cat");
        exit(EXIT_FAILURE);
    }

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
            const char *arg = argv[i];
            if (arg[0] == '<' && arg[1] != '\0')
                arg++;

            char path[PATH_MAX];
            size_t len = strlen(arg);
            if (len == PATH_MAX) {
                fprintf(stderr, "cat: %s: File name too long\n", arg);
                status = 1;
                continue;
            }
            memcpy(path, arg, len + 1);
            fd = open(path, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "cat: %s: %s\n",
                        arg, strerror(errno));
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