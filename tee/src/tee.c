#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int append = 0;

    int c;
    while ((c = getopt(argc, argv, "ai")) != -1) {
        switch (c) {
        case 'a': append = 1; break;
        case 'i': break;
        default:
            fprintf(stderr, "usage: tee [-a] [file...]\n");
            return 1;
        }
    }

    int status = 0;
    int n = argc - optind;
    int *fds = calloc(n + 1, sizeof(int));
    int nfds = 0;

    fds[nfds++] = STDOUT_FILENO;
    for (int i = optind; i < argc; i++) {
        int fd = open(argv[i], O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC));
        if (fd < 0) {
            status = warn("%s: %s", argv[i], strerror(errno));
            continue;
        }
        fds[nfds++] = fd;
    }

    char buf[4096];
    for (;;) {
        ssize_t r = read(STDIN_FILENO, buf, sizeof(buf));
        if (r < 0 && errno == EINTR)
            continue;
        if (r <= 0)
            break;
        for (int i = 0; i < nfds; i++) {
            if (fds[i] >= 0 && write_all(fds[i], buf, (size_t)r) < 0) {
                status = warn("write error: %s", strerror(errno));
                if (fds[i] != STDOUT_FILENO)
                    close(fds[i]);
                fds[i] = -1;
            }
        }
    }

    for (int i = 1; i < nfds; i++) {
        if (fds[i] >= 0)
            close(fds[i]);
    }
    free(fds);
    return status;
}
