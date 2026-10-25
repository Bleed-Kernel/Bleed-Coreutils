#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int no_create = 0;

    int c;
    while ((c = getopt(argc, argv, "acmd:r:t:")) != -1) {
        switch (c) {
        case 'c':
            no_create = 1;
            break;
        case 'a': case 'm': case 'd': case 'r': case 't':
            // the filesystems dont let us set times yet
            break;
        default:
            fprintf(stderr, "usage: touch [-c] file...\n");
            return 1;
        }
    }

    if (optind >= argc) {
        warn("missing file operand");
        return 1;
    }

    int status = 0;
    for (int i = optind; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == 0 || no_create)
            continue;

        int fd = open(argv[i], O_WRONLY | O_CREAT);
        if (fd < 0) {
            status = warn("cannot touch '%s': %s", argv[i], strerror(errno));
            continue;
        }
        close(fd);
    }
    return status;
}
